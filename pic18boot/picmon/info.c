/*********************************************************************
 *	‚t‚r‚aƒfƒoƒCƒX‚Ìî•ñ‚ğ•\¦‚·‚é.
 *********************************************************************
 *
 *‚`‚o‚h:
void	dump_info(struct usb_device *dev );
 *
 *“à•”ŠÖ”:
void	dump_dev_info(struct usb_device_descriptor *d);
void	dump_ep_info(struct usb_endpoint_descriptor *e ,int cnt);
void	dump_iface_info(struct usb_interface *d,int cnt);
void	dump_cfg_info(struct usb_config_descriptor *d);
 *
 */


#include <stdio.h>
#include <time.h>
#include <usb.h>

void dump_dev_info(struct usb_device_descriptor *d)
{
	printf("usb_device_descriptor:\n");
	{
		printf("%20s:%d\n","bLength",			d->bLength);
		printf("%20s:%d\n","bDescriptorType",	d->bDescriptorType);
		printf("%20s:0x%x\n","bcdUSB",			d->bcdUSB		);
		printf("%20s:%d\n","bDeviceClass",		d->bDeviceClass		);
		printf("%20s:%d\n","bDeviceSubClass",	d->bDeviceSubClass		);
		printf("%20s:%d\n","bDeviceProtocol",	d->bDeviceProtocol		);
		printf("%20s:%d\n","bMaxPacketSize0",	d->bMaxPacketSize0		);
		printf("%20s:0x%x\n","idVendor",			d->idVendor		);
		printf("%20s:0x%x\n","idProduct",			d->idProduct		);
		printf("%20s:0x%x\n","bcdDevice",			d->bcdDevice		);
		printf("%20s:%d\n","iManufacturer",		d->iManufacturer		);
		printf("%20s:%d\n","iProduct",			d->iProduct		);
		printf("%20s:%d\n","iSerialNumber",		d->iSerialNumber		);
		printf("%20s:%d\n","bNumConfigurations",d->bNumConfigurations		);
		printf("\n");
	}
}

void dump_ep_info(struct usb_endpoint_descriptor *e ,int cnt)
{
	int i;
	for(i=0;i<cnt;i++,e++) {
	printf("usb_endpoint_descriptor %d:\n",1+i);

	printf("%20s:%d\n","bLength"			,e->bLength	);
	printf("%20s:%d\n","bDescriptorType"	,e->bDescriptorType	);
	printf("%20s:0x%02x\n","bEndpointAddress"	,e->bEndpointAddress	);
	printf("%20s:0x%02x\n","bmAttributes"		,e->bmAttributes	);
	printf("%20s:%d\n","wMaxPacketSize"		,e->wMaxPacketSize	);
	printf("%20s:%d\n","bInterval"			,e->bInterval	);
	printf("%20s:%d\n","bRefresh"			,e->bRefresh	);
	printf("%20s:%d\n","bSynchAddress"		,e->bSynchAddress	);
	printf("%20s:%d\n","extralen"			,e->extralen	);
	printf("\n");
	}
}


void dump_iface_info(struct usb_interface *d,int cnt)
{
	int i;
	for(i=0;i<cnt;i++,d++) {
	printf("usb_interface %d:\n",1+i);

	printf("%20s:%d\n","num_altsetting",d->num_altsetting	);
  	struct usb_interface_descriptor *a = d->altsetting ;
	if(a) {
		printf("%20s:%d\n","bLength",a->bLength	);
		printf("%20s:%d\n","bDescriptorType",a->bDescriptorType	);
		printf("%20s:%d\n","bInterfaceNumber",a->bInterfaceNumber	);
		printf("%20s:%d\n","bAlternateSetting",a->bAlternateSetting	);
		printf("%20s:%d\n","bNumEndpoints",a->bNumEndpoints	);
		printf("%20s:%d\n","bInterfaceClass",a->bInterfaceClass	);
		printf("%20s:%d\n","bInterfaceSubClass",a->bInterfaceSubClass	);
		printf("%20s:%d\n","bInterfaceProtocol",a->bInterfaceProtocol	);
		printf("%20s:%d\n","iInterface",a->iInterface	);
		printf("%20s:%d\n","extralen",a->extralen	);
		printf("\n");

  		struct usb_endpoint_descriptor *e = a->endpoint;
		if(e) {
			dump_ep_info(e,a->bNumEndpoints	);
		}
	}
	}
}
	
void dump_cfg_info(struct usb_config_descriptor *d)
{
	printf("usb_config_descriptor:\n");
	{
		printf("%20s:%d\n","bLength",d->bLength	);
		printf("%20s:%d\n","bDescriptorType",d->bDescriptorType	);
		printf("%20s:%d\n","wTotalLength",d->wTotalLength	);
		printf("%20s:%d\n","bNumInterfaces",d->bNumInterfaces	);
		printf("%20s:%d\n","bConfigurationValue",d->bConfigurationValue	);
		printf("%20s:%d\n","iConfiguration",d->iConfiguration	);
		printf("%20s:%d\n","bmAttributes",d->bmAttributes	);
		printf("%20s:%d\n","MaxPower",d->MaxPower	);
		printf("%20s:%d\n","extralen",d->extralen	);
		printf("\n");
	}

	dump_iface_info(d->interface,d->bNumInterfaces	);
}

void dump_info(  struct usb_device *dev )
{
  	struct usb_device_descriptor *d = &dev->descriptor;
  	struct usb_config_descriptor *c = dev->config;

	dump_dev_info(d);
	dump_cfg_info(c);
}

/*********************************************************************
 *
 *********************************************************************
 */

