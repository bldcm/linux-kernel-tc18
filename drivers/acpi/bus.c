/*
 *  acpi_bus.c - ACPI Bus Driver ($Revision$)
 *
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#ifdef CONFIG_X86
#include <asm/mpspec.h>
#endif
#include <acpi/acpi_bus.h>
#include <acpi/acpi_drivers.h>


#define _COMPONENT		ACPI_BUS_COMPONENT
ACPI_MODULE_NAME		("acpi_bus")

#ifdef	CONFIG_X86
extern void __init acpi_pic_sci_set_trigger(unsigned int irq, u16 trigger);
#endif

FADT_DESCRIPTOR			acpi_fadt;
EXPORT_SYMBOL(acpi_fadt);

struct acpi_device		*acpi_root;
struct proc_dir_entry		*acpi_root_dir;
EXPORT_SYMBOL(acpi_root_dir);

#define STRUCT_TO_INT(s)	(*((int*)&s))

/* --------------------------------------------------------------------------
                                Device Management
   -------------------------------------------------------------------------- */

int
acpi_bus_get_device (
	acpi_handle		handle,
	struct acpi_device	**device)
{
	acpi_status		status = AE_OK;

	ACPI_FUNCTION_TRACE("acpi_bus_get_device");

	if (!device)
		return_VALUE(-EINVAL);

	/* TBD: Support fixed-feature devices */

	status = acpi_get_data(handle, acpi_bus_data_handler, (void**) device);
	if (ACPI_FAILURE(status) || !*device) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "No context for object [%p]\n",
			handle));
		return_VALUE(-ENODEV);
	}

	return_VALUE(0);
}
EXPORT_SYMBOL(acpi_bus_get_device);

int
acpi_bus_get_status (
	struct acpi_device	*device)
{
	acpi_status		status = AE_OK;
	unsigned long		sta = 0;
	
	ACPI_FUNCTION_TRACE("acpi_bus_get_status");

	if (!device)
		return_VALUE(-EINVAL);

	/*
	 * Evaluate _STA if present.
	 */
	if (device->flags.dynamic_status) {
		status = acpi_evaluate_integer(device->handle, "_STA", NULL, &sta);
		if (ACPI_FAILURE(status))
			return_VALUE(-ENODEV);
		STRUCT_TO_INT(device->status) = (int) sta;
	}

	/*
	 * Otherwise we assume the status of our parent (unless we don't
	 * have one, in which case status is implied).
	 */
	else if (device->parent)
		device->status = device->parent->status;
	else
		STRUCT_TO_INT(device->status) = 0x0F;

	if (device->status.functional && !device->status.present) {
		printk(KERN_WARNING PREFIX "Device [%s] status [%08x]: "
			"functional but not present; setting present\n",
			device->pnp.bus_id,
			(u32) STRUCT_TO_INT(device->status));
		device->status.present = 1;
	}

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] status [%08x]\n", 
		device->pnp.bus_id, (u32) STRUCT_TO_INT(device->status)));

	return_VALUE(0);
}
EXPORT_SYMBOL(acpi_bus_get_status);


/* --------------------------------------------------------------------------
                                 Power Management
   -------------------------------------------------------------------------- */

int
acpi_bus_get_power (
	acpi_handle		handle,
	int			*state)
{
	int			result = 0;
	acpi_status             status = 0;
	struct acpi_device	*device = NULL;
	unsigned long		psc = 0;

	ACPI_FUNCTION_TRACE("acpi_bus_get_power");

	result = acpi_bus_get_device(handle, &device);
	if (result)
		return_VALUE(result);

	*state = ACPI_STATE_UNKNOWN;

	if (!device->flags.power_manageable) {
		/* TBD: Non-recursive algorithm for walking up hierarchy */
		if (device->parent)
			*state = device->parent->power.state;
		else
			*state = ACPI_STATE_D0;
	}
	else {
		/*
		 * Get the device's power state either directly (via _PSC) or 
		 * indirectly (via power resources).
		 */
		if (device->power.flags.explicit_get) {
			status = acpi_evaluate_integer(device->handle, "_PSC", 
				NULL, &psc);
			if (ACPI_FAILURE(status))
				return_VALUE(-ENODEV);
			device->power.state = (int) psc;
		}
		else if (device->power.flags.power_resources) {
			result = acpi_power_get_inferred_state(device);
			if (result)
				return_VALUE(result);
		}

		*state = device->power.state;
	}

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] power state is D%d\n",
		device->pnp.bus_id, device->power.state));

	return_VALUE(0);
}
EXPORT_SYMBOL(acpi_bus_get_power);


int
acpi_bus_set_power (
	acpi_handle		handle,
	int			state)
{
	int			result = 0;
	acpi_status		status = AE_OK;
	struct acpi_device	*device = NULL;
	char			object_name[5] = {'_','P','S','0'+state,'\0'};

	ACPI_FUNCTION_TRACE("acpi_bus_set_power");

	result = acpi_bus_get_device(handle, &device);
	if (result)
		return_VALUE(result);

	if ((state < ACPI_STATE_D0) || (state > ACPI_STATE_D3))
		return_VALUE(-EINVAL);

	/* Make sure this is a valid target state */

	if (!device->flags.power_manageable) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Device is not power manageable\n"));
		return_VALUE(-ENODEV);
	}
	if (state == device->power.state) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device is already at D%d\n", state));
		return_VALUE(0);
	}
	if (!device->power.states[state].flags.valid) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Device does not support D%d\n", state));
		return_VALUE(-ENODEV);
	}
	if (device->parent && (state < device->parent->power.state)) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Cannot set device to a higher-powered state than parent\n"));
		return_VALUE(-ENODEV);
	}

	/*
	 * Transition Power
	 * ----------------
	 * On transitions to a high-powered state we first apply power (via
	 * power resources) then evalute _PSx.  Conversly for transitions to
	 * a lower-powered state.
	 */ 
	if (state < device->power.state) {
		if (device->power.flags.power_resources) {
			result = acpi_power_transition(device, state);
			if (result)
				goto end;
		}
		if (device->power.states[state].flags.explicit_set) {
			status = acpi_evaluate_object(device->handle, 
				object_name, NULL, NULL);
			if (ACPI_FAILURE(status)) {
				result = -ENODEV;
				goto end;
			}
		}
	}
	else {
		if (device->power.states[state].flags.explicit_set) {
			status = acpi_evaluate_object(device->handle, 
				object_name, NULL, NULL);
			if (ACPI_FAILURE(status)) {
				result = -ENODEV;
				goto end;
			}
		}
		if (device->power.flags.power_resources) {
			result = acpi_power_transition(device, state);
			if (result)
				goto end;
		}
	}

end:
	if (result)
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Error transitioning device [%s] to D%d\n",
			device->pnp.bus_id, state));
	else
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] transitioned to D%d\n",
			device->pnp.bus_id, state));

	return_VALUE(result);
}
EXPORT_SYMBOL(acpi_bus_set_power);



/* --------------------------------------------------------------------------
                                Event Management
   -------------------------------------------------------------------------- */

static DEFINE_SPINLOCK(acpi_bus_event_lock);

LIST_HEAD(acpi_bus_event_list);
DECLARE_WAIT_QUEUE_HEAD(acpi_bus_event_queue);

extern int			event_is_open;

int
acpi_bus_generate_event (
	struct acpi_device	*device,
	u8			type,
	int			data)
{
	struct acpi_bus_event	*event = NULL;
	unsigned long		flags = 0;

	ACPI_FUNCTION_TRACE("acpi_bus_generate_event");

	if (!device)
		return_VALUE(-EINVAL);

	/* drop event on the floor if no one's listening */
	if (!event_is_open)
		return_VALUE(0);

	event = kmalloc(sizeof(struct acpi_bus_event), GFP_ATOMIC);
	if (!event)
		return_VALUE(-ENOMEM);

	strcpy(event->device_class, device->pnp.device_class);
	strcpy(event->bus_id, device->pnp.bus_id);
	event->type = type;
	event->data = data;

	spin_lock_irqsave(&acpi_bus_event_lock, flags);
	list_add_tail(&event->node, &acpi_bus_event_list);
	spin_unlock_irqrestore(&acpi_bus_event_lock, flags);

	wake_up_interruptible(&acpi_bus_event_queue);

	return_VALUE(0);
}
EXPORT_SYMBOL(acpi_bus_generate_event);

int
acpi_bus_receive_event (
	struct acpi_bus_event	*event)
{
	unsigned long		flags = 0;
	struct acpi_bus_event	*entry = NULL;

	DECLARE_WAITQUEUE(wait, current);

	ACPI_FUNCTION_TRACE("acpi_bus_receive_event");

	if (!event)
		return_VALUE(-EINVAL);

	if (list_empty(&acpi_bus_event_list)) {

		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&acpi_bus_event_queue, &wait);

		if (list_empty(&acpi_bus_event_list))
			schedule();

		remove_wait_queue(&acpi_bus_event_queue, &wait);
		set_current_state(TASK_RUNNING);

		if (signal_pending(current))
			return_VALUE(-ERESTARTSYS);
	}

	spin_lock_irqsave(&acpi_bus_event_lock, flags);
	entry = list_entry(acpi_bus_event_list.next, struct acpi_bus_event, node);
	if (entry)
		list_del(&entry->node);
	spin_unlock_irqrestore(&acpi_bus_event_lock, flags);

	if (!entry)
		return_VALUE(-ENODEV);

	memcpy(event, entry, sizeof(struct acpi_bus_event));

	kfree(entry);

	return_VALUE(0);
}
EXPORT_SYMBOL(acpi_bus_receive_event);


/* --------------------------------------------------------------------------
                             Notification Handling
   -------------------------------------------------------------------------- */

static int
acpi_bus_check_device (
	struct acpi_device	*device,
	int			*status_changed)
{
	acpi_status             status = 0;
	struct acpi_device_status old_status;

	ACPI_FUNCTION_TRACE("acpi_bus_check_device");

	if (!device)
		return_VALUE(-EINVAL);

	if (status_changed)
		*status_changed = 0;

	old_status = device->status;

	/*
	 * Make sure this device's parent is present before we go about
	 * messing with the device.
	 */
	if (device->parent && !device->parent->status.present) {
		device->status = device->parent->status;
		if (STRUCT_TO_INT(old_status) != STRUCT_TO_INT(device->status)) {
			if (status_changed)
				*status_changed = 1;
		}
		return_VALUE(0);
	}

	status = acpi_bus_get_status(device);
	if (ACPI_FAILURE(status))
		return_VALUE(-ENODEV);

	if (STRUCT_TO_INT(old_status) == STRUCT_TO_INT(device->status))
		return_VALUE(0);

	if (status_changed)
		*status_changed = 1;
	
	/*
	 * Device Insertion/Removal
	 */
	if ((device->status.present) && !(old_status.present)) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device insertion detected\n"));
		/* TBD: Handle device insertion */
	}
	else if (!(device->status.present) && (old_status.present)) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device removal detected\n"));
		/* TBD: Handle device removal */
	}

	return_VALUE(0);
}


static int
acpi_bus_check_scope (
	struct acpi_device	*device)
{
	int			result = 0;
	int			status_changed = 0;

	ACPI_FUNCTION_TRACE("acpi_bus_check_scope");

	if (!device)
		return_VALUE(-EINVAL);

	/* Status Change? */
	result = acpi_bus_check_device(device, &status_changed);
	if (result)
		return_VALUE(result);

	if (!status_changed)
		return_VALUE(0);

	/*
	 * TBD: Enumerate child devices within this device's scope and
	 *       run acpi_bus_check_device()'s on them.
	 */

	return_VALUE(0);
}


/**
 * acpi_bus_notify
 * ---------------
 * Callback for all 'system-level' device notifications (values 0x00-0x7F).
 */
static void 
acpi_bus_notify (
	acpi_handle             handle,
	u32                     type,
	void                    *data)
{
	int			result = 0;
	struct acpi_device	*device = NULL;

	ACPI_FUNCTION_TRACE("acpi_bus_notify");

	if (acpi_bus_get_device(handle, &device))
		return_VOID;

	switch (type) {

	case ACPI_NOTIFY_BUS_CHECK:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received BUS CHECK notification for device [%s]\n", 
			device->pnp.bus_id));
		result = acpi_bus_check_scope(device);
		/* 
		 * TBD: We'll need to outsource certain events to non-ACPI
		 *	drivers via the device manager (device.c).
		 */
		break;

	case ACPI_NOTIFY_DEVICE_CHECK:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received DEVICE CHECK notification for device [%s]\n", 
			device->pnp.bus_id));
		result = acpi_bus_check_device(device, NULL);
		/* 
		 * TBD: We'll need to outsource certain events to non-ACPI
		 *	drivers via the device manager (device.c).
		 */
		break;

	case ACPI_NOTIFY_DEVICE_WAKE:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received DEVICE WAKE notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD */
		break;

	case ACPI_NOTIFY_EJECT_REQUEST:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received EJECT REQUEST notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD */
		break;

	case ACPI_NOTIFY_DEVICE_CHECK_LIGHT:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received DEVICE CHECK LIGHT notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD: Exactly what does 'light' mean? */
		break;

	case ACPI_NOTIFY_FREQUENCY_MISMATCH:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received FREQUENCY MISMATCH notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD */
		break;

	case ACPI_NOTIFY_BUS_MODE_MISMATCH:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received BUS MODE MISMATCH notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD */
		break;

	case ACPI_NOTIFY_POWER_FAULT:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received POWER FAULT notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD */
		break;

	default:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received unknown/unsupported notification [%08x]\n", 
			type));
		break;
	}

	return_VOID;
}

/* --------------------------------------------------------------------------
                             Initialization/Cleanup
   -------------------------------------------------------------------------- */

static int __init
acpi_bus_init_irq (void)
{
	acpi_status		status = AE_OK;
	union acpi_object	arg = {ACPI_TYPE_INTEGER};
	struct acpi_object_list	arg_list = {1, &arg};
	char			*message = NULL;

	ACPI_FUNCTION_TRACE("acpi_bus_init_irq");

	/* 
	 * Let the system know what interrupt model we are using by
	 * evaluating the \_PIC object, if exists.
	 */

	switch (acpi_irq_model) {
	case ACPI_IRQ_MODEL_PIC:
		message = "PIC";
		break;
	case ACPI_IRQ_MODEL_IOAPIC:
		message = "IOAPIC";
		break;
	case ACPI_IRQ_MODEL_IOSAPIC:
		message = "IOSAPIC";
		break;
	default:
		printk(KERN_WARNING PREFIX "Unknown interrupt routing model\n");
		return_VALUE(-ENODEV);
	}

	printk(KERN_INFO PREFIX "Using %s for interrupt routing\n", message);

	arg.integer.value = acpi_irq_model;

	status = acpi_evaluate_object(NULL, "\\_PIC", &arg_list, NULL);
	if (ACPI_FAILURE(status) && (status != AE_NOT_FOUND)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error evaluating _PIC\n"));
		return_VALUE(-ENODEV);
	}

	return_VALUE(0);
}


void __init
acpi_early_init (void)
{
	acpi_status		status = AE_OK;
	struct acpi_buffer	buffer = {sizeof(acpi_fadt), &acpi_fadt};

	ACPI_FUNCTION_TRACE("acpi_early_init");

	if (acpi_disabled)
		return_VOID;

	/* enable workarounds, unless strict ACPI spec. compliance */
	if (!acpi_strict)
		acpi_gbl_enable_interpreter_slack = TRUE;

	status = acpi_initialize_subsystem();
	if (ACPI_FAILURE(status)) {
		printk(KERN_ERR PREFIX "Unable to initialize the ACPI Interpreter\n");
		goto error0;
	}

	status = acpi_load_tables();
	if (ACPI_FAILURE(status)) {
		printk(KERN_ERR PREFIX "Unable to load the System Description Tables\n");
		goto error0;
	}

	/*
	 * Get a separate copy of the FADT for use by other drivers.
	 */
	status = acpi_get_table(ACPI_TABLE_FADT, 1, &buffer);
	if (ACPI_FAILURE(status)) {
		printk(KERN_ERR PREFIX "Unable to get the FADT\n");
		goto error0;
	}

#ifdef CONFIG_X86
	if (!acpi_ioapic) {
		extern acpi_interrupt_flags acpi_sci_flags;

		/* compatible (0) means level (3) */
		if (acpi_sci_flags.trigger == 0)
			acpi_sci_flags.trigger = 3;

		/* Set PIC-mode SCI trigger type */
		acpi_pic_sci_set_trigger(acpi_fadt.sci_int, acpi_sci_flags.trigger);
	} else {
		extern int acpi_sci_override_gsi;
		/*
		 * now that acpi_fadt is initialized,
		 * update it with result from INT_SRC_OVR parsing
		 */
		acpi_fadt.sci_int = acpi_sci_override_gsi;
	}
#endif

	status = acpi_enable_subsystem(~(ACPI_NO_HARDWARE_INIT | ACPI_NO_ACPI_ENABLE));
	if (ACPI_FAILURE(status)) {
		printk(KERN_ERR PREFIX "Unable to enable ACPI\n");
		goto error0;
	}

	return_VOID;

error0:
	disable_acpi();
	return_VOID;
}

static int __init
acpi_bus_init (void)
{
	int			result = 0;
	acpi_status		status = AE_OK;
	extern acpi_status	acpi_os_initialize1(void);

	ACPI_FUNCTION_TRACE("acpi_bus_init");

	status = acpi_os_initialize1();

	status = acpi_enable_subsystem(ACPI_NO_HARDWARE_INIT | ACPI_NO_ACPI_ENABLE);
	if (ACPI_FAILURE(status)) {
		printk(KERN_ERR PREFIX "Unable to start the ACPI Interpreter\n");
		goto error1;
	}

	if (ACPI_FAILURE(status)) {
		printk(KERN_ERR PREFIX "Unable to initialize ACPI OS objects\n");
		goto error1;
	}
#ifdef CONFIG_ACPI_EC
	/*
	 * ACPI 2.0 requires the EC driver to be loaded and work before
	 * the EC device is found in the namespace (i.e. before acpi_initialize_objects()
	 * is called).
	 *
	 * This is accomplished by looking for the ECDT table, and getting 
	 * the EC parameters out of that.
	 */
	status = acpi_ec_ecdt_probe();
	/* Ignore result. Not having an ECDT is not fatal. */
#endif

	status = acpi_initialize_objects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		printk(KERN_ERR PREFIX "Unable to initialize ACPI objects\n");
		goto error1;
	}

	printk(KERN_INFO PREFIX "Interpreter enabled\n");

	/*
	 * Get the system interrupt model and evaluate \_PIC.
	 */
	result = acpi_bus_init_irq();
	if (result)
		goto error1;

	/*
	 * Register the for all standard device notifications.
	 */
	status = acpi_install_notify_handler(ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY, &acpi_bus_notify, NULL);
	if (ACPI_FAILURE(status)) {
		printk(KERN_ERR PREFIX "Unable to register for device notifications\n");
		goto error1;
	}

	/*
	 * Create the top ACPI proc directory
	 */
	acpi_root_dir = proc_mkdir(ACPI_BUS_FILE_ROOT, NULL);

	return_VALUE(0);

	/* Mimic structured exception handling */
error1:
	acpi_terminate();
	return_VALUE(-ENODEV);
}

decl_subsys(acpi,NULL,NULL);

static int __init acpi_init (void)
{
	int			result = 0;

	ACPI_FUNCTION_TRACE("acpi_init");

	printk(KERN_INFO PREFIX "Subsystem revision %08x\n",
		ACPI_CA_VERSION);

	if (acpi_disabled) {
		printk(KERN_INFO PREFIX "Interpreter disabled.\n");
		return_VALUE(-ENODEV);
	}

	firmware_register(&acpi_subsys);

	result = acpi_bus_init();

	if (!result) {
#ifdef CONFIG_PM
		if (!PM_IS_ACTIVE())
			pm_active = 1;
		else {
			printk(KERN_INFO PREFIX "APM is already active, exiting\n");
			disable_acpi();
			result = -ENODEV;
		}
#endif
	} else
		disable_acpi();

	return_VALUE(result);
}

subsys_initcall(acpi_init);
