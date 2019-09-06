/*
 * ppc/usb.h:
 *
 */
#ifndef _PPC_USB_H
#define _PPC_USB_H

struct usb_hcd_platform_data {
	int (*start) (struct platform_device *pdev);
	void (*stop) (struct platform_device *pdev);
};

#endif /* !(_PPC_USB_H) */
