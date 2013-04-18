/*
 *
 * arch/arm/mach-meson/meson.c
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
 *
 * License terms: GNU General Public License (GPL) version 2
 * Platform machine definition.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/device.h>
#include <linux/spi/flash.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/memory.h>
#include <mach/clock.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <mach/lm.h>
#include <asm/memory.h>
#include <asm/mach/map.h>
#include <mach/nand.h>
#include <linux/i2c.h>
#include <linux/i2c-aml.h>
#include <mach/power_gate.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <mach/usbclock.h>
#include <linux/aml_eth.h>

#ifdef CONFIG_AM_UART_WITH_S_CORE 
#include <linux/uart-aml.h>
#endif
#include <mach/card_io.h>
#include <mach/pinmux.h>
#include <mach/gpio.h>
#include <linux/delay.h>
#include <mach/clk_set.h>
#include "board-m3-reff16.h"

#ifdef CONFIG_ANDROID_PMEM
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/android_pmem.h>
#endif

#ifdef CONFIG_AMLOGIC_PM
#include <linux/power_supply.h>
#include <linux/aml_power.h>
#endif

#ifdef CONFIG_SUSPEND
#include <mach/pm.h>
#endif

#ifdef CONFIG_EFUSE
#include <linux/efuse.h>
#endif

#ifdef CONFIG_AML_HDMI_TX
#include <linux/hdmi/hdmi_config.h>
#endif

/* GPIO Defines */
// LEDS
#define GPIO_LED_POWER GPIO_AO(10)
#define GPIO_LED_STATUS GPIO_AO(11)
// ETHERNET
#define GPIO_ETH_RESET GPIO_D(7)
// POWERSUPPLIES
#define GPIO_PWR_VCCIO GPIO_AO(2)
#define GPIO_PWR_VCCx2 GPIO_AO(6)
#define GPIO_PWR_HDMI  GPIO_D(6)
#define GPIO_PWR_WIFI  GPIO_C(3)
// WIFI
#define GPIO_INT_WIFI  GPIO_C(3)
#define GPIO_CLK_WIFI  GPIO_C(15)

#define NET_EXT_CLK

#ifdef CONFIG_SUSPEND
static int suspend_state=0;
#endif

#if defined(CONFIG_JPEGLOGO)
static struct resource jpeglogo_resources[] = {
    [0] = {
        .start = CONFIG_JPEGLOGO_ADDR,
        .end   = CONFIG_JPEGLOGO_ADDR + CONFIG_JPEGLOGO_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = CODEC_ADDR_START,
        .end   = CODEC_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device jpeglogo_device = {
    .name = "jpeglogo-dev",
    .id   = 0,
    .num_resources = ARRAY_SIZE(jpeglogo_resources),
    .resource      = jpeglogo_resources,
};
#endif

#if defined(CONFIG_KEYPADS_AM)||defined(CONFIG_KEYPADS_AM_MODULE)
static struct resource intput_resources[] = {
    {
        .start = 0x0,
        .end = 0x0,
        .name="8726M3",
        .flags = IORESOURCE_IO,
    },
};

static struct platform_device input_device = {
    .name = "m1-kp",
    .id = 0,
    .num_resources = ARRAY_SIZE(intput_resources),
    .resource = intput_resources,
};
#endif

#ifdef CONFIG_SARADC_AM
#include <linux/saradc.h>
static struct platform_device saradc_device = {
    .name = "saradc",
    .id = 0,
    .dev = {
        .platform_data = NULL,
    },
};
#endif

#if defined(CONFIG_ADC_KEYPADS_AM)||defined(CONFIG_ADC_KEYPADS_AM_MODULE)
#include <linux/input.h>
#include <linux/adc_keypad.h>

static struct adc_key adc_kp_key[] = {
    {KEY_RECOVERY, "menu", CHAN_4, 0, 60},
};

static struct adc_kp_platform_data adc_kp_pdata = {
    .key = &adc_kp_key[0],
    .key_num = ARRAY_SIZE(adc_kp_key),
};

static struct platform_device adc_kp_device = {
    .name = "m1-adckp",
    .id = 0,
    .num_resources = 0,
    .resource = NULL,
    .dev = {
    .platform_data = &adc_kp_pdata,
    }
};
#endif

#if defined(CONFIG_AM_IR_RECEIVER)
#include <linux/input/irreceiver.h>

static int ir_init()
{
    unsigned int control_value;
    
    //mask--mux gpioao_7 to remote
    SET_AOBUS_REG_MASK(AO_RTI_PIN_MUX_REG,1<<0);
    
    //max frame time is 80ms, base rate is 20us
    control_value = 3<<28|(0x9c40 << 12)|0x13;
    WRITE_AOBUS_REG(AO_IR_DEC_REG0, control_value);
     
    /*[3-2]rising or falling edge detected
      [8-7]Measure mode
    */
    control_value = 0x8574;
    WRITE_AOBUS_REG(AO_IR_DEC_REG1, control_value);
    
    return 0;
}

static int pwm_init()
{
	CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_7, (1<<16));
	CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_0, (1<<29));
	SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_3, (1<<26));
}

static struct irreceiver_platform_data irreceiver_data = {
    .pwm_no = PWM_A,
    .freq = 38000, //38k
    .init_pwm_pinmux = pwm_init,
    .init_ir_pinmux = ir_init,
};

static struct platform_device irreceiver_device = {
    .name = "irreceiver",
    .id = 0,
    .num_resources = 0,
    .resource = NULL,
    .dev = {
        .platform_data = &irreceiver_data,
    }
};
#endif

#if defined(CONFIG_FB_AM)
static struct resource fb_device_resources[] = {
    [0] = {
        .start = OSD1_ADDR_START,
        .end   = OSD1_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
#if defined(CONFIG_FB_OSD2_ENABLE)
    [1] = {
        .start = OSD2_ADDR_START,
        .end   = OSD2_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
#endif
};

static struct platform_device fb_device = {
    .name       = "mesonfb",
    .id         = 0,
    .num_resources = ARRAY_SIZE(fb_device_resources),
    .resource      = fb_device_resources,
};
#endif

#ifdef CONFIG_USB_DWC_OTG_HCD
#ifdef CONFIG_USB_DPLINE_PULLUP_DISABLE
static set_vbus_valid_ext_fun(unsigned int id,char val)
{
	unsigned int  reg = (PREI_USB_PHY_A_REG1 + id);
	if(val == 1)
	{
		SET_CBUS_REG_MASK(reg,1<<0);
	}
	else
	{
		CLEAR_CBUS_REG_MASK(reg,1<<0);
	}
}
#endif
static void set_usb_a_vbus_power(char is_power_on)
{
}

static void set_usb_b_vbus_power(char is_power_on)
{
}


 /*wifi rtl8188cus power control*/
/*
#define USB_B_POW_GPIO         GPIOC_bank_bit0_15(5)
#define USB_B_POW_GPIO_BIT     GPIOC_bit_bit0_15(5)
#define USB_B_POW_GPIO_BIT_ON   1
#define USB_B_POW_GPIO_BIT_OFF  0
    if(is_power_on) {
        printk(KERN_INFO "set usb b port power on (board gpio %d)!\n",USB_B_POW_GPIO_BIT);
        set_gpio_mode(USB_B_POW_GPIO, USB_B_POW_GPIO_BIT, GPIO_OUTPUT_MODE);
        set_gpio_val(USB_B_POW_GPIO, USB_B_POW_GPIO_BIT, USB_B_POW_GPIO_BIT_ON);
    } else    {
        printk(KERN_INFO "set usb b port power off (board gpio %d)!\n",USB_B_POW_GPIO_BIT);
        set_gpio_mode(USB_B_POW_GPIO, USB_B_POW_GPIO_BIT, GPIO_OUTPUT_MODE);
        set_gpio_val(USB_B_POW_GPIO, USB_B_POW_GPIO_BIT, USB_B_POW_GPIO_BIT_OFF);
    }
}
*/

static struct lm_device usb_ld_a = {
    .type = LM_DEVICE_TYPE_USB,
    .id = 0,
    .irq = INT_USB_A,
    .resource.start = IO_USB_A_BASE,
    .resource.end = -1,
    .dma_mask_room = DMA_BIT_MASK(32),
    .port_type = USB_PORT_TYPE_HOST,
    .port_speed = USB_PORT_SPEED_DEFAULT,
    .dma_config = USB_DMA_BURST_SINGLE,
    .set_vbus_power = set_usb_a_vbus_power,
#ifdef CONFIG_USB_DPLINE_PULLUP_DISABLE
    .set_vbus_valid_ext = set_vbus_valid_ext_fun,
#endif
};
static struct lm_device usb_ld_b = {
    .type = LM_DEVICE_TYPE_USB,
    .id = 1,
    .irq = INT_USB_B,
    .resource.start = IO_USB_B_BASE,
    .resource.end = -1,
    .dma_mask_room = DMA_BIT_MASK(32),
    .port_type = USB_PORT_TYPE_HOST,
    .port_speed = USB_PORT_SPEED_DEFAULT,
    .dma_config = USB_DMA_BURST_SINGLE,
    .set_vbus_power = set_usb_b_vbus_power,
#ifdef CONFIG_USB_DPLINE_PULLUP_DISABLE
    .set_vbus_valid_ext = set_vbus_valid_ext_fun,
#endif	
};

#endif

#if defined(CONFIG_AM_STREAMING)
static struct resource codec_resources[] = {
    [0] = {
	.start =  CODEC_ADDR_START,
	.end   = CODEC_ADDR_END,
	.flags = IORESOURCE_MEM,
    },
    [1] = {
	.start = STREAMBUF_ADDR_START,
	.end = STREAMBUF_ADDR_END,
	.flags = IORESOURCE_MEM,
    },
};

static struct platform_device codec_device = {
    .name       = "amstream",
    .id         = 0,
    .num_resources = ARRAY_SIZE(codec_resources),
    .resource      = codec_resources,
};
#endif

#if defined(CONFIG_AM_VIDEO)
static struct resource deinterlace_resources[] = {
    [0] = {
        .start =  DI_ADDR_START,
        .end   = DI_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device deinterlace_device = {
    .name       = "deinterlace",
    .id         = 0,
    .num_resources = ARRAY_SIZE(deinterlace_resources),
    .resource      = deinterlace_resources,
};
#endif


#if defined(CONFIG_AM_NAND)

/*** Default NAND layout for HD18M ***
0M    0x000000000000-0x000000400000 : "bootloader" : 4M
8M 				    : Empty Space  : 8M
12M   0x000000c00000-0x000001000000 : "ubootenv"   : 4M
16M   0x000001000000-0x000002000000 : "aml_logo"   : 16M
32M   0x000002000000-0x000003000000 : "recovery"   : 16M
48M   0x000003000000-0x000004400000 : "boot"       : 20M
68M   0x000004400000-0x000024400000 : "system"     : 512M
580M  0x000024400000-0x000030400000 : "cache"      : 192M
772M  0x000030400000-0x000070400000 : "userdata"   : 3324M
*/

static struct mtd_partition multi_partition_info[] = { // 4G
/* Hide uboot partition
	{//4M for uboot
		.name   = "uboot",
		.offset = 0,
		.size   = 4*1024*1024,
	},
//*/
{//4M for ubootenv
	.name   = "ubootenv",
	.offset = 12*1024*1024,
	.size   = 4*1024*1024,
},
{//16M for logo
	.name   = "aml_logo",
	.offset = 16*1024*1024,
	.size   = 16*1024*1024,
},
/* Hide recovery partition
{//16M for recovery
	.name   = "recovery",
	.offset = 32*1024*1024,
	.size   = 16*1024*1024,
},
//*/
{//20M for kernel
	.name   = "boot",
	.offset = (48)*1024*1024,
	.size   = 20*1024*1024,
},
{//512M for android system.
	.name   = "system",
	.offset = (48+20)*1024*1024,
	.size   = 512*1024*1024,
},
{//192M for cache
	.name   = "cache",
	.offset = (48+20+512)*1024*1024,
	.size   = 192*1024*1024,
},
{//3324M for userdata
	.name   = "userdata",
	.offset = MTDPART_OFS_APPEND,
	.size   = MTDPART_SIZ_FULL,
},
};

static void nand_set_parts(uint64_t size, struct platform_nand_chip *chip)
{
    printk("set nand parts for chip %lldMB\n", (size/(1024*1024)));

    chip->partitions = multi_partition_info;
    chip->nr_partitions = ARRAY_SIZE(multi_partition_info);
}

static struct aml_nand_platform aml_nand_mid_platform[] = {
#ifdef CONFIG_AML_NAND_ENV
{
		.name = NAND_BOOT_NAME,
		.chip_enable_pad = AML_NAND_CE0,
		.ready_busy_pad = AML_NAND_CE0,
		.platform_nand_data = {
			.chip =  {
				.nr_chips = 1,
				.options = (NAND_TIMING_MODE5 | NAND_ECC_BCH60_1K_MODE),
			},
    	},
			.T_REA = 20,
			.T_RHOH = 15,
	},
#endif
{
		.name = NAND_MULTI_NAME,
		.chip_enable_pad = (AML_NAND_CE0 | (AML_NAND_CE1 << 4) | (AML_NAND_CE2 << 8) | (AML_NAND_CE3 << 12)),
		.ready_busy_pad = (AML_NAND_CE0 | (AML_NAND_CE0 << 4) | (AML_NAND_CE1 << 8) | (AML_NAND_CE1 << 12)),
		.platform_nand_data = {
			.chip =  {
				.nr_chips = 4,
				.nr_partitions = ARRAY_SIZE(multi_partition_info),
				.partitions = multi_partition_info,
				.set_parts = nand_set_parts,
				.options = (NAND_TIMING_MODE5 | NAND_ECC_BCH60_1K_MODE | NAND_TWO_PLANE_MODE),
			},
    	},
			.T_REA = 20,
			.T_RHOH = 15,
	}
};

struct aml_nand_device aml_nand_mid_device = {
	.aml_nand_platform = aml_nand_mid_platform,
	.dev_num = ARRAY_SIZE(aml_nand_mid_platform),
};

static struct resource aml_nand_resources[] = {
    {
        .start = 0xc1108600,
        .end = 0xc1108624,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device aml_nand_device = {
    .name = "aml_m3_nand",
    .id = 0,
    .num_resources = ARRAY_SIZE(aml_nand_resources),
    .resource = aml_nand_resources,
    .dev = {
		.platform_data = &aml_nand_mid_device,
    },
};
#endif

#if defined(CONFIG_SDIO_DHD_CDC_WIFI_40181_MODULE)
/******************************
*WL_REG_ON	-->GPIOC_8
*WIFI_32K		-->GPIOC_15(CLK_OUT1)
*WIFIWAKE(WL_HOST_WAKE)-->GPIOX_11
*******************************/
//#define WL_REG_ON_USE_GPIOC_6
void extern_wifi_set_enable(int enable)
{
	if(enable){
#ifdef WL_REG_ON_USE_GPIOC_6
		SET_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<6));
#else
		SET_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<8));
#endif
		printk("Enable WIFI  Module!\n");
	}
    	else{
#ifdef WL_REG_ON_USE_GPIOC_6
		CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<6));
#else
		CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<8));
#endif
		printk("Disable WIFI  Module!\n");
	}
}
EXPORT_SYMBOL(extern_wifi_set_enable);

static void wifi_set_clk_enable(int on)
{
	//set clk for wifi
	printk("set WIFI CLK Pin GPIOC_15 32KHz ***%d\n",on);
	WRITE_CBUS_REG(HHI_GEN_CLK_CNTL,(READ_CBUS_REG(HHI_GEN_CLK_CNTL)&(~(0x7f<<0)))|((0<<0)|(1<<8)|(7<<9)) );
	CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_EN_N, (1<<15));   
	if(on)
		SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_3, (1<<22));
	else
		CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_3, (1<<22));
}

static void wifi_gpio_init(void)
{
#ifdef WL_REG_ON_USE_GPIOC_6
    //set WL_REG_ON Pin GPIOC_6 out
        CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_0, (1<<16));
        CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_1, (1<<5));
        CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_EN_N, (1<<6));  //GPIOC_6
#else
    //set WL_REG_ON Pin GPIOC_8 out 
   	CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_3, (1<<23));
	CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_0, (1<<18));
	CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_1, (1<<10));
     	CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_EN_N, (1<<8));  //GPIOC_8
#endif
}


static void aml_wifi_bcm4018x_init(void)
{
	wifi_set_clk_enable(1);
	wifi_gpio_init();
	extern_wifi_set_enable(0);
        msleep(5);
	extern_wifi_set_enable(1);
}

#endif

#if defined(CONFIG_CARDREADER)
static struct resource amlogic_card_resource[] = {
    [0] = {
        .start = 0x1200230,   //physical address
        .end   = 0x120024c,
        .flags = 0x200,
    }
};

#if defined(CONFIG_SDIO_DHD_CDC_WIFI_40181_MODULE)
#define GPIO_WIFI_HOSTWAKE  ((GPIOX_bank_bit0_31(11)<<16) |GPIOX_bit_bit0_31(11))
void sdio_extern_init(void)
{
	printk("sdio_extern_init !\n");
	SET_CBUS_REG_MASK(PAD_PULL_UP_REG4, (1<<11));
	gpio_direction_input(GPIO_WIFI_HOSTWAKE);
#if defined(CONFIG_BCM40181_WIFI)
	gpio_enable_level_int(gpio_to_idx(GPIO_WIFI_HOSTWAKE), 0, 4);  //for 40181
#endif
#if defined(CONFIG_BCM40183_WIFI)
	gpio_enable_edge_int(gpio_to_idx(GPIO_WIFI_HOSTWAKE), 0, 5);     //for 40183
#endif 
	//extern_wifi_set_enable(1);
}
#endif

static struct aml_card_info  amlogic_card_info[] = {
    [0] = {
        .name = "sd_card",
        .work_mode = CARD_HW_MODE,
        .io_pad_type = SDIO_B_CARD_0_5,
        .card_ins_en_reg = CARD_GPIO_ENABLE,
        .card_ins_en_mask = PREG_IO_29_MASK,
        .card_ins_input_reg = CARD_GPIO_INPUT,
        .card_ins_input_mask = PREG_IO_29_MASK,
        .card_power_en_reg = CARD_GPIO_ENABLE,
        .card_power_en_mask = PREG_IO_31_MASK,
        .card_power_output_reg = CARD_GPIO_OUTPUT,
        .card_power_output_mask = PREG_IO_31_MASK,
        .card_power_en_lev = 0,
        .card_wp_en_reg = CARD_GPIO_ENABLE,
        .card_wp_en_mask = PREG_IO_30_MASK,
        .card_wp_input_reg = CARD_GPIO_INPUT,
        .card_wp_input_mask = PREG_IO_30_MASK,
        .card_extern_init = 0,
    },
#if defined(CONFIG_SDIO_DHD_CDC_WIFI_40181_MODULE)
    [1] = {
        .name = "sdio_card",
        .work_mode = CARD_HW_MODE,
        .io_pad_type = SDIO_A_GPIOX_0_3,
        .card_ins_en_reg = 0,
        .card_ins_en_mask = 0,
        .card_ins_input_reg = 0,
        .card_ins_input_mask = 0,
        .card_power_en_reg = 0,
        .card_power_en_mask = 0,
        .card_power_output_reg = 0,
        .card_power_output_mask = 0,
        .card_power_en_lev = 1,
        .card_wp_en_reg = 0,
        .card_wp_en_mask = 0,
        .card_wp_input_reg = 0,
        .card_wp_input_mask = 0,
        .card_extern_init = sdio_extern_init,
    },
#endif
};

static struct aml_card_platform amlogic_card_platform = {
    .card_num = ARRAY_SIZE(amlogic_card_info),
    .card_info = amlogic_card_info,
};

static struct platform_device amlogic_card_device = { 
    .name = "AMLOGIC_CARD",
    .id    = -1,
    .num_resources = ARRAY_SIZE(amlogic_card_resource),
    .resource = amlogic_card_resource,
    .dev = {
        .platform_data = &amlogic_card_platform,
    },
};

#endif

#if defined(CONFIG_AML_AUDIO_DSP)
static struct resource audiodsp_resources[] = {
    [0] = {
        .start = AUDIODSP_ADDR_START,
        .end   = AUDIODSP_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device audiodsp_device = {
    .name       = "audiodsp",
    .id         = 0,
    .num_resources = ARRAY_SIZE(audiodsp_resources),
    .resource      = audiodsp_resources,
};
#endif

static struct resource aml_m3_audio_resource[] = {
    [0] =   {
        .start  =   0,
        .end        =   0,
        .flags  =   IORESOURCE_MEM,
    },
};

#if defined(CONFIG_SND_AML_M3)
static struct platform_device aml_audio = {
    .name               = "aml_m3_audio",
    .id                     = -1,
    .resource       =   aml_m3_audio_resource,
    .num_resources  =   ARRAY_SIZE(aml_m3_audio_resource),
};

int aml_m3_is_hp_pluged(void)
{
	return 0; //return 1: hp pluged, 0: hp unpluged.
}

void mute_spk(void* codec, int flag)
{
}

#define APB_BASE 0x4000
void mute_headphone(void* codec, int flag)
{
	int reg_val;
#ifdef _AML_M3_HW_DEBUG_
	printk("***Entered %s:%s\n", __FILE__,__func__);
#endif
	reg_val = READ_APB_REG(APB_BASE+(0x18<<2));
    if(flag){
		reg_val |= 0xc0;
		WRITE_APB_REG((APB_BASE+(0x18<<2)), reg_val);			// mute headphone
	}else{
		reg_val &= ~0xc0;
		WRITE_APB_REG((APB_BASE+(0x18<<2)), reg_val);			// unmute headphone
	}
}

#endif

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_data =
{
    .name = "pmem",
    .start = PMEM_START,
    .size = PMEM_SIZE,
    .no_allocator = 1,
    .cached = 0,
};

static struct platform_device android_pmem_device =
{
    .name = "android_pmem",
    .id = 0,
    .dev = {
        .platform_data = &pmem_data,
    },
};
#endif

#if defined(CONFIG_AML_RTC)
static  struct platform_device aml_rtc_device = {
            .name            = "aml_rtc",
            .id               = -1,
    };
#endif

#if defined (CONFIG_AMLOGIC_VIDEOIN_MANAGER)
static struct resource vm_resources[] = {
    [0] = {
        .start =  VM_ADDR_START,
        .end   = VM_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};
static struct platform_device vm_device =
{
	.name = "vm",
	.id = 0,
    .num_resources = ARRAY_SIZE(vm_resources),
    .resource      = vm_resources,
};
#endif /* AMLOGIC_VIDEOIN_MANAGER */

#if defined(CONFIG_SUSPEND)
typedef struct {
	char name[32];
	unsigned bank;
	unsigned bit;
	gpio_mode_t mode;
	unsigned value;
	unsigned enable;
} gpio_data_t;

#define MAX_GPIO 4
static gpio_data_t gpio_data[MAX_GPIO] = {
{"GPIOD6--HDMI", 	GPIOD_bank_bit0_9(6), 	GPIOD_bit_bit0_9(6), 	GPIO_OUTPUT_MODE, 1, 1},
{"GPIOD9--VCC5V", GPIOD_bank_bit0_9(9), 	GPIOD_bit_bit0_9(9), 	GPIO_OUTPUT_MODE, 1, 1},
{"GPIOX29--MUTE", 	GPIOX_bank_bit0_31(29), GPIOX_bit_bit0_31(29), GPIO_OUTPUT_MODE, 1, 1},
//{"GPIOC7--SATA", 	GPIOC_bank_bit0_15(7), GPIOC_bit_bit0_15(7), GPIO_OUTPUT_MODE, 1, 1},

{"GPIOA10--LED", 	GPIOAO_bank_bit0_11(10), GPIOAO_bit_bit0_11(10), GPIO_OUTPUT_MODE, 0, 1},
//{"GPIOA10--WIFI", GPIOC_bank_bit0_15(10), GPIOC_bit_bit0_15(10), GPIO_OUTPUT_MODE, 0, 1},
};	

static void save_gpio(int port) 
{
	gpio_data[port].mode = get_gpio_mode(gpio_data[port].bank, gpio_data[port].bit);
	if (gpio_data[port].mode==GPIO_OUTPUT_MODE)
	{
		if (gpio_data[port].enable){
			printk("change %s output %d to input\n", gpio_data[port].name, gpio_data[port].value); 
			gpio_data[port].value = get_gpio_val(gpio_data[port].bank, gpio_data[port].bit);
			set_gpio_mode(gpio_data[port].bank, gpio_data[port].bit, GPIO_INPUT_MODE);
		}
		else{
			printk("no change %s output %d\n", gpio_data[port].name, gpio_data[port].value); 
		}
	}
}

static void restore_gpio(int port)
{
	if ((gpio_data[port].mode==GPIO_OUTPUT_MODE)&&(gpio_data[port].enable))
	{
		set_gpio_val(gpio_data[port].bank, gpio_data[port].bit, gpio_data[port].value);
		set_gpio_mode(gpio_data[port].bank, gpio_data[port].bit, GPIO_OUTPUT_MODE);
		// printk("%s output %d\n", gpio_data[port].name, gpio_data[port].value); 
	}
}

typedef struct {
	char name[32];
	unsigned reg;
	unsigned bits;
	unsigned enable;
} pinmux_data_t;


#define MAX_PINMUX	1

pinmux_data_t pinmux_data[MAX_PINMUX] = {
	{"HDMI", 	0, (1<<2)|(1<<1)|(1<<0), 						1},
};

static unsigned pinmux_backup[6];

static void save_pinmux(void)
{
	int i;
	for (i=0;i<6;i++)
		pinmux_backup[i] = READ_CBUS_REG(PERIPHS_PIN_MUX_0+i);
	for (i=0;i<MAX_PINMUX;i++){
		if (pinmux_data[i].enable){
			printk("%s %x\n", pinmux_data[i].name, pinmux_data[i].bits);
			clear_mio_mux(pinmux_data[i].reg, pinmux_data[i].bits);
		}
	}
}

static void restore_pinmux(void)
{
	int i;
	for (i=0;i<6;i++)
		 WRITE_CBUS_REG(PERIPHS_PIN_MUX_0+i, pinmux_backup[i]);
}

static void set_vccx2(int power_on)
{
	int i;

	if (power_on){
		restore_pinmux();
		for (i=0;i<MAX_GPIO;i++)
			restore_gpio(i);
		printk(KERN_INFO "set_vcc power up\n");

#ifdef CONFIG_AML_SUSPEND
		suspend_state=10;
#endif
		/*wifi */	
		set_gpio_mode(GPIOC_bank_bit0_15(6), GPIOC_bit_bit0_15(6), GPIO_OUTPUT_MODE);
		set_gpio_val(GPIOC_bank_bit0_15(6), GPIOC_bit_bit0_15(6), 0);
	}
	else{
		printk(KERN_INFO "set_vcc power down\n");       			
		/*wifi */	
		set_gpio_mode(GPIOC_bank_bit0_15(6), GPIOC_bit_bit0_15(6), GPIO_OUTPUT_MODE);
		set_gpio_val(GPIOC_bank_bit0_15(6), GPIOC_bit_bit0_15(6), 0);
		for (i=0;i<MAX_GPIO;i++)
			save_gpio(i);
	}
}

extern void hdmi_wr_reg(unsigned long addr, unsigned long data);

static void set_gpio_suspend_resume(int power_on)
{
	if(power_on) {
		printk("set gpio resume.\n");
		 // HDMI
		hdmi_wr_reg(0x8005, 2); 
		udelay(50);
		hdmi_wr_reg(0x8005, 1); 
	}
	else
	{
		printk("set gpio suspend.\n");
	}
}

static struct meson_pm_config aml_pm_pdata = {
	.pctl_reg_base	= (void __iomem *)IO_APB_BUS_BASE,
	.mmc_reg_base	= (void __iomem *)APB_REG_ADDR(0x1000),
	.hiu_reg_base	= (void __iomem *)CBUS_REG_ADDR(0x1000),
	.power_key	= (1<<8),
	.ddr_clk 	= 0x00110820,
	.sleepcount	= 128,
	.set_vccx2	= set_vccx2,
	.core_voltage_adjust = 7,  //5,8
	.set_exgpio_early_suspend = set_gpio_suspend_resume,
};

static struct platform_device aml_pm_device = {
    .name           = "pm-meson",
    .dev = {
        .platform_data  = &aml_pm_pdata,
    },
    .id             = -1,
};
#endif

#if defined(CONFIG_I2C_SW_AML)
#define MESON3_I2C_PREG_GPIOX_OE		CBUS_REG_ADDR(PREG_PAD_GPIO4_EN_N)
#define MESON3_I2C_PREG_GPIOX_OUTLVL	CBUS_REG_ADDR(PREG_PAD_GPIO4_O)
#define MESON3_I2C_PREG_GPIOX_INLVL	CBUS_REG_ADDR(PREG_PAD_GPIO4_I)

static struct aml_sw_i2c_platform aml_sw_i2c_plat = {
    .sw_pins = {
        .scl_reg_out        = MESON3_I2C_PREG_GPIOX_OUTLVL,
        .scl_reg_in     = MESON3_I2C_PREG_GPIOX_INLVL,
        .scl_bit            = 26, 
        .scl_oe         = MESON3_I2C_PREG_GPIOX_OE,
        .sda_reg_out        = MESON3_I2C_PREG_GPIOX_OUTLVL,
        .sda_reg_in     = MESON3_I2C_PREG_GPIOX_INLVL,
        .sda_bit            = 25,
        .sda_oe         = MESON3_I2C_PREG_GPIOX_OE,
    },  
    .udelay         = 2,
    .timeout            = 100,
};

#if 0
static struct aml_sw_i2c_platform aml_sw_i2c_plat = {
    .sw_pins = {
        .scl_reg_out        = MESON_I2C_PREG_GPIOB_OUTLVL,
        .scl_reg_in     = MESON_I2C_PREG_GPIOB_INLVL,
        .scl_bit            = 2,    /*MESON_I2C_MASTER_A_GPIOB_2_REG*/
        .scl_oe         = MESON_I2C_PREG_GPIOB_OE,
        .sda_reg_out        = MESON_I2C_PREG_GPIOB_OUTLVL,
        .sda_reg_in     = MESON_I2C_PREG_GPIOB_INLVL,
        .sda_bit            = 3,    /*MESON_I2C_MASTER_A_GPIOB_3_BIT*/
        .sda_oe         = MESON_I2C_PREG_GPIOB_OE,
    },  
    .udelay         = 2,
    .timeout            = 100,
};
#endif
static struct platform_device aml_sw_i2c_device = {
    .name         = "aml-sw-i2c",
    .id       = -1,
    .dev = {
        .platform_data = &aml_sw_i2c_plat,
    },
};

#endif

#if defined(CONFIG_I2C_AML) || defined(CONFIG_I2C_HW_AML)
static struct aml_i2c_platform aml_i2c_plat = {
    .wait_count     = 50000,
    .wait_ack_interval  = 5,
    .wait_read_interval = 5,
    .wait_xfer_interval = 5,
//    .master_no      = 0,
    .use_pio            = 0,
    .master_i2c_speed   = AML_I2C_SPPED_300K,

    .master_pinmux = {
        .scl_reg    = MESON_I2C_MASTER_GPIOX_26_REG,
        .scl_bit    = MESON_I2C_MASTER_GPIOX_26_BIT,
        .sda_reg    = MESON_I2C_MASTER_GPIOX_25_REG,
        .sda_bit    = MESON_I2C_MASTER_GPIOX_25_BIT,
    }
};

static struct aml_i2c_platform aml_i2c_plat1 = {
    .wait_count     = 50000,
    .wait_ack_interval  = 5,
    .wait_read_interval = 5,
    .wait_xfer_interval = 5,
//    .master_no      = 1,
    .use_pio            = 0,
    .master_i2c_speed   = AML_I2C_SPPED_300K,

    .master_pinmux = {
        .scl_reg    = MESON_I2C_MASTER_GPIOX_28_REG,
        .scl_bit    = MESON_I2C_MASTER_GPIOX_28_BIT,
        .sda_reg    = MESON_I2C_MASTER_GPIOX_27_REG,
        .sda_bit    = MESON_I2C_MASTER_GPIOX_27_BIT,
    }
};

static struct aml_i2c_platform aml_i2c_plat2 = {
    .wait_count     = 50000,
    .wait_ack_interval  = 5,
    .wait_read_interval = 5,
    .wait_xfer_interval = 5,
//    .master_no      = 2,
    .use_pio            = 0,
    .master_i2c_speed   = AML_I2C_SPPED_300K,

    .master_pinmux = {
        .scl_reg    = MESON_I2C_MASTER_GPIOAO_4_REG,
        .scl_bit    = MESON_I2C_MASTER_GPIOAO_4_BIT,
        .sda_reg    = MESON_I2C_MASTER_GPIOAO_5_REG,
        .sda_bit    = MESON_I2C_MASTER_GPIOAO_5_BIT,
    }
};

static struct resource aml_i2c_resource[] = {
	[0]= {
		.start =    MESON_I2C_MASTER_A_START,
		.end   =    MESON_I2C_MASTER_A_END,
		.flags =    IORESOURCE_MEM,
	}
};

static struct resource aml_i2c_resource1[] = {
	[0]= {
		.start =    MESON_I2C_MASTER_A_START,
		.end   =    MESON_I2C_MASTER_A_END,
		.flags =    IORESOURCE_MEM,
  }
};

static struct resource aml_i2c_resource2[] = {
	[0]= {
		.start =    MESON_I2C_MASTER_AO_START,
		.end   =    MESON_I2C_MASTER_AO_END,
		.flags =    IORESOURCE_MEM,
	}
};

static struct platform_device aml_i2c_device = {
    .name         = "aml-i2c",
    .id       = 0,
    .num_resources    = ARRAY_SIZE(aml_i2c_resource),
    .resource     = aml_i2c_resource,
    .dev = {
        .platform_data = &aml_i2c_plat,
    },
};

static struct platform_device aml_i2c_device1 = {
    .name         = "aml-i2c",
    .id       = 1,
    .num_resources    = ARRAY_SIZE(aml_i2c_resource1),
    .resource     = aml_i2c_resource1,
    .dev = {
        .platform_data = &aml_i2c_plat1,
    },
};

static struct platform_device aml_i2c_device2 = {
    .name         = "aml-i2c",
    .id       = 2,
    .num_resources    = ARRAY_SIZE(aml_i2c_resource2),
    .resource     = aml_i2c_resource2,
    .dev = {
        .platform_data = &aml_i2c_plat2,
    },
};

#endif

#ifdef CONFIG_AMLOGIC_PM

static int is_ac_connected(void)
{
    return 1;
}

static void set_charge(int flags)
{
}

#ifdef CONFIG_SARADC_AM
extern int get_adc_sample(int chan);
#endif
static int get_bat_vol(void)
{
#ifdef CONFIG_SARADC_AM
    return get_adc_sample(5);
#else
        return 0;
#endif
}

static int get_charge_status(void)
{
    return 0;
}

static void power_off(void)
{
    //Power hold down
    //set_gpio_val(GPIOAO_bank_bit0_11(6), GPIOAO_bit_bit0_11(6), 0);
    //set_gpio_mode(GPIOAO_bank_bit0_11(6), GPIOAO_bit_bit0_11(6), GPIO_OUTPUT_MODE);
}

static void set_bat_off(void)
{
    //BL_PWM power off
    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_2, (1<<31));
    CLEAR_CBUS_REG_MASK(PWM_MISC_REG_AB, (1 << 0));
    //set_gpio_val(GPIOA_bank_bit(7), GPIOA_bit_bit0_14(7), 0);
    //set_gpio_mode(GPIOA_bank_bit(7), GPIOA_bit_bit0_14(7), GPIO_OUTPUT_MODE);

    //VCCx2 power down
#if defined(CONFIG_SUSPEND)
    set_vccx2(0);
#endif
}

static int bat_value_table[37]={
0,  //0    
737,//0
742,//4
750,//10
752,//15
753,//16
754,//18
755,//20
756,//23
757,//26
758,//29
760,//32
761,//35
762,//37
763,//40
766,//43
768,//46
770,//49
772,//51
775,//54
778,//57
781,//60
786,//63
788,//66
791,//68
795,//71
798,//74
800,//77
806,//80
810,//83
812,//85
817,//88
823,//91
828,//95
832,//97
835,//100
835 //100
};

static int bat_charge_value_table[37]={
0,  //0    
766,//0
773,//4
779,//10
780,//15
781,//16
782,//18
783,//20
784,//23
785,//26
786,//29
787,//32
788,//35
789,//37
790,//40
791,//43
792,//46
794,//49
796,//51
798,//54
802,//57
804,//60
807,//63
809,//66
810,//68
813,//71
815,//74
818,//77
820,//80
823,//83
825,//85
828,//88
831,//91
836,//95
839,//97
842,//100
842 //100
}; 

static int bat_level_table[37]={
0,
0,
4,
10,
15,
16,
18,
20,
23,
26,
29,
32,
35,
37,
40,
43,
46,
49,
51,
54,
57,
60,
63,
66,
68,
71,
74,
77,
80,
83,
85,
88,
91,
95,
97,
100,
100  
};

static struct aml_power_pdata power_pdata = {
	.is_ac_online	= is_ac_connected,
	//.is_usb_online	= is_usb_connected,
	.set_charge = set_charge,
	.get_bat_vol = get_bat_vol,
	.get_charge_status = get_charge_status,
	.set_bat_off = set_bat_off,
	.bat_value_table = bat_value_table,
	.bat_charge_value_table = bat_charge_value_table,
	.bat_level_table = bat_level_table,
	.bat_table_len = 37,		
	.is_support_usb_charging = 0,
	//.supplied_to = supplicants,
	//.num_supplicants = ARRAY_SIZE(supplicants),
};

static struct platform_device power_dev = {
    .name       = "aml-power",
    .id     = -1,
    .dev = {
        .platform_data  = &power_pdata,
    },
};
#endif

#if defined(CONFIG_AM_UART_WITH_S_CORE)
static struct aml_uart_platform aml_uart_plat = {
    .uart_line[0]       =   UART_AO,
    .uart_line[1]       =   UART_A,
    .uart_line[2]       =   UART_B,
    .uart_line[3]       =   UART_C
};

static struct platform_device aml_uart_device = {
    .name         = "am_uart",  
    .id       = -1, 
    .num_resources    = 0,  
    .resource     = NULL,   
    .dev = {        
                .platform_data = &aml_uart_plat,
           },
};
#endif

#ifdef CONFIG_EFUSE
static bool efuse_data_verify(unsigned char *usid)
{  int len;
  
    len = strlen(usid);
    if((len > 8)&&(len<31) )
        return true;
		else
				return false;
}

static struct efuse_platform_data aml_efuse_plat = {
    .pos = 337,
    .count = 30,
    .data_verify = efuse_data_verify,
};

static struct platform_device aml_efuse_device = {
    .name	= "efuse",
    .id	= -1,
    .dev = {
                .platform_data = &aml_efuse_plat,
           },
};
#endif


#if  defined(CONFIG_AM_TV_OUTPUT)||defined(CONFIG_AM_TCON_OUTPUT)
static struct resource vout_device_resources[] = {
    [0] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device vout_device = {
    .name       = "mesonvout",
    .id         = 0,
    .num_resources = ARRAY_SIZE(vout_device_resources),
    .resource      = vout_device_resources,
};
#endif

#ifdef CONFIG_POST_PROCESS_MANAGER
static struct resource ppmgr_resources[] = {
    [0] = {
        .start =  PPMGR_ADDR_START,
        .end   = PPMGR_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};
static struct platform_device ppmgr_device = {
    .name       = "ppmgr",
    .id         = 0,
    .num_resources = ARRAY_SIZE(ppmgr_resources),
    .resource      = ppmgr_resources,
};
#endif
#ifdef CONFIG_FREE_SCALE
static struct resource freescale_resources[] = {
    [0] = {
        .start = FREESCALE_ADDR_START,
        .end   = FREESCALE_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device freescale_device =
{
    .name           = "freescale",
    .id             = 0,
    .num_resources  = ARRAY_SIZE(freescale_resources),
    .resource       = freescale_resources,
};
#endif

#ifdef CONFIG_BT_DEVICE
#include <linux/bt-device.h>

static struct platform_device bt_device = {
	.name             = "bt-dev",
	.id               = -1,
};

static void bt_device_init(void)
{
}

static void bt_device_on(void)
{
}

static void bt_device_off(void)
{
}

struct bt_dev_data bt_dev = {
    .bt_dev_init    = bt_device_init,
    .bt_dev_on      = bt_device_on,
    .bt_dev_off     = bt_device_off,
};
#endif

#if defined(CONFIG_AML_WATCHDOG)
static struct platform_device aml_wdt_device = {
    .name = "aml_wdt",
    .id   = -1,
    .num_resources = 0,
};
#endif

#define ETH_PM_DEV
#if defined(ETH_PM_DEV)
#define ETH_MODE_RMII_EXTERNAL
static void meson_eth_clock_enable(int flag)
{
    printk("meson_eth_clock_enable: %x\n", (unsigned int)flag );
}

static void meson_eth_reset(void)
{
    printk("meson_eth_reset\n");
    // Ethernet Reset, GPIO D7, ACTIVE LOW
    gpio_direction_output(GPIO_ETH_RESET, 0);
    mdelay(100);
    gpio_set_value(GPIO_ETH_RESET, 1);
}

static struct aml_eth_platform_data aml_pm_eth_platform_data = {
	.clock_enable = meson_eth_clock_enable,
	.reset = meson_eth_reset,
};

struct platform_device meson_device_eth = {
	.name = "ethernet_pm_driver",
	.id = -1,
	.dev = {
		.platform_data = &aml_pm_eth_platform_data,
	}
};
#endif

#if defined(CONFIG_AML_HDMI_TX)
static struct hdmi_phy_set_data brd_phy_data[] = {
//    {27, 0xf7, 0x0},    // an example: set Reg0xf7 to 0 in 27MHz
    {-1,   -1},         //end of phy setting
};
static struct hdmi_config_platform_data aml_hdmi_pdata ={
    .hdmi_5v_ctrl = NULL,
    .hdmi_3v3_ctrl = NULL,
    .hdmi_pll_vdd_ctrl = NULL,
    .hdmi_sspll_ctrl = NULL,
    .phy_data = brd_phy_data,
};

static struct platform_device aml_hdmi_device = {
    .name = "amhdmitx",
    .id   = -1,
    .dev  = {
        .platform_data = &aml_hdmi_pdata,
    }
};
#endif

static struct platform_device __initdata *platform_devs[] = {
#if defined(CONFIG_AML_HDMI_TX)
    &aml_hdmi_device,
#endif
#if defined(CONFIG_JPEGLOGO)
    &jpeglogo_device,
#endif
#if defined(CONFIG_FB_AM)
    &fb_device,
#endif
#if defined(CONFIG_AM_STREAMING)
    &codec_device,
#endif
#if defined(CONFIG_AM_VIDEO)
    &deinterlace_device,
#endif
#if defined(CONFIG_AML_AUDIO_DSP)
    &audiodsp_device,
#endif
#if defined(CONFIG_SND_AML_M3)
    &aml_audio,
#endif
#if defined(CONFIG_CARDREADER)
    &amlogic_card_device,
#endif
#if defined(CONFIG_KEYPADS_AM)||defined(CONFIG_VIRTUAL_REMOTE)||defined(CONFIG_KEYPADS_AM_MODULE)
    &input_device,
#endif
#ifdef CONFIG_SARADC_AM
    &saradc_device,
#endif
#if defined(CONFIG_ADC_KEYPADS_AM)||defined(CONFIG_ADC_KEYPADS_AM_MODULE)
    &adc_kp_device,
#endif
//#if defined(CONFIG_KEY_INPUT_CUSTOM_AM) || defined(CONFIG_KEY_INPUT_CUSTOM_AM_MODULE)
    // &input_device_key,  //changed by Elvis
//#endif
#if defined(CONFIG_AM_IR_RECEIVER)
    &irreceiver_device,
#endif
#ifdef CONFIG_AM_NAND
    &aml_nand_device,
#endif
#ifdef CONFIG_BT_DEVICE
 	&bt_device,
#endif
#if defined(CONFIG_AML_RTC)
    &aml_rtc_device,
#endif
#ifdef CONFIG_AMLOGIC_VIDEOIN_MANAGER
	&vm_device,
#endif
#if defined(CONFIG_SUSPEND)
    &aml_pm_device,
#endif
#if defined(CONFIG_ANDROID_PMEM)
    &android_pmem_device,
#endif
#if defined(CONFIG_AM_UART_WITH_S_CORE)
    &aml_uart_device,
#endif
#if defined(CONFIG_AM_TV_OUTPUT)||defined(CONFIG_AM_TCON_OUTPUT)
    &vout_device,   
#endif
#ifdef CONFIG_POST_PROCESS_MANAGER
    &ppmgr_device,
#endif
#ifdef CONFIG_FREE_SCALE
        &freescale_device,
#endif   
#ifdef CONFIG_EFUSE
	&aml_efuse_device,
#endif
#if defined(CONFIG_AML_WATCHDOG)
        &aml_wdt_device,
#endif
};

static void __init eth_pinmux_init(void)
{
    /* Setup Ethernet and Pinmux */
    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6,(3<<17));//reg6[17/18]=0
#ifdef NET_EXT_CLK
    eth_set_pinmux(ETH_BANK0_GPIOY1_Y9, ETH_CLK_IN_GPIOY0_REG6_18, 0);
#else
    eth_set_pinmux(ETH_BANK0_GPIOY1_Y9, ETH_CLK_OUT_GPIOY0_REG6_17, 0);
#endif

    CLEAR_CBUS_REG_MASK(PREG_ETHERNET_ADDR0, 1); // Disable the Ethernet clocks
    // ---------------------------------------------
    // Test 50Mhz Input Divide by 2
    // ---------------------------------------------
    // Select divide by 2
    CLEAR_CBUS_REG_MASK(PREG_ETHERNET_ADDR0, (1<<3)); // desc endianess "same order"
    CLEAR_CBUS_REG_MASK(PREG_ETHERNET_ADDR0, (1<<2)); // ata endianess "little"
    SET_CBUS_REG_MASK(PREG_ETHERNET_ADDR0, (1 << 1)); // divide by 2 for 100M
    SET_CBUS_REG_MASK(PREG_ETHERNET_ADDR0, 1); // enable Ethernet clocks
    udelay(100);

    /* Reset Ethernet */
    meson_eth_reset();
}

static void __init device_pinmux_init(void )
{
    clearall_pinmux();
    eth_pinmux_init();
#if defined(CONFIG_SDIO_DHD_CDC_WIFI_40181_MODULE)
    aml_wifi_bcm4018x_init();
#endif
}

static void __init  device_clk_setting(void)
{
	/*eth clk*/
#ifdef NET_EXT_CLK
	eth_clk_set(ETH_CLKSRC_EXT_XTAL_CLK, (50 * CLK_1M), (50 * CLK_1M), 1);
#else    
	eth_clk_set(ETH_CLKSRC_MISC_CLK, get_misc_pll_clk(), (50 * CLK_1M), 0);
#endif
}

static void disable_unused_model(void)
{
    CLK_GATE_OFF(VIDEO_IN);
    CLK_GATE_OFF(BT656_IN);
//    CLK_GATE_OFF(ETHERNET);
//    CLK_GATE_OFF(SATA);
//    CLK_GATE_OFF(WIFI);
    video_dac_disable();
}

static void __init power_hold(void)
{
	printk(KERN_INFO "power hold set high!\n");

        // VCC5V
        set_gpio_mode(GPIOD_bank_bit0_9(9), GPIOD_bit_bit0_9(9), GPIO_OUTPUT_MODE);
        set_gpio_val(GPIOD_bank_bit0_9(9), GPIOD_bit_bit0_9(9), 1);
	
	// hdmi power on
        set_gpio_mode(GPIOD_bank_bit0_9(6), GPIOD_bit_bit0_9(6), GPIO_OUTPUT_MODE);
        set_gpio_val(GPIOD_bank_bit0_9(6), GPIOD_bit_bit0_9(6), 1);

	// VCC, set to high when suspend 
        set_gpio_mode(GPIOAO_bank_bit0_11(4), GPIOAO_bit_bit0_11(4), GPIO_OUTPUT_MODE);
        set_gpio_val(GPIOAO_bank_bit0_11(4), GPIOAO_bit_bit0_11(4), 0);
        set_gpio_mode(GPIOAO_bank_bit0_11(5), GPIOAO_bit_bit0_11(5), GPIO_OUTPUT_MODE);
        set_gpio_val(GPIOAO_bank_bit0_11(5), GPIOAO_bit_bit0_11(5), 0);

	// VCCK
        set_gpio_mode(GPIOAO_bank_bit0_11(6), GPIOAO_bit_bit0_11(6), GPIO_OUTPUT_MODE);
        set_gpio_val(GPIOAO_bank_bit0_11(6), GPIOAO_bit_bit0_11(6), 1);
	// VCCIO
        set_gpio_mode(GPIOAO_bank_bit0_11(2), GPIOAO_bit_bit0_11(2), GPIO_OUTPUT_MODE);
        set_gpio_val(GPIOAO_bank_bit0_11(2), GPIOAO_bit_bit0_11(2), 1);
}

#ifdef CONFIG_AML_SUSPEND
extern int (*pm_power_suspend)(void);
#endif /*CONFIG_AML_SUSPEND*/

static __init void m1_init_machine(void)
{
	meson_cache_init();
#ifdef CONFIG_AML_SUSPEND
	pm_power_suspend = meson_power_suspend;
#endif /*CONFIG_AML_SUSPEND*/
        power_hold();
        device_clk_setting();
        device_pinmux_init();
	platform_add_devices(platform_devs, ARRAY_SIZE(platform_devs));

#ifdef CONFIG_USB_DWC_OTG_HCD
	printk("***m1_init_machine: usb set mode.\n");
	set_usb_phy_clk(USB_PHY_CLOCK_SEL_XTAL_DIV2);
	set_usb_phy_id_mode(USB_PHY_PORT_A, USB_PHY_MODE_SW_HOST);
	lm_device_register(&usb_ld_a);
	set_usb_phy_id_mode(USB_PHY_PORT_B,USB_PHY_MODE_SW_HOST);
	lm_device_register(&usb_ld_b);
#endif
	disable_unused_model();
}

/*VIDEO MEMORY MAPING*/
static __initdata struct map_desc meson_video_mem_desc[] = {
    {
        .virtual    = PAGE_ALIGN(__phys_to_virt(RESERVED_MEM_START)),
        .pfn        = __phys_to_pfn(RESERVED_MEM_START),
        .length     = RESERVED_MEM_END-RESERVED_MEM_START+1,
        .type       = MT_DEVICE,
    },
#ifdef CONFIG_AML_SUSPEND
    {
        .virtual    = PAGE_ALIGN(0xdff00000),
        .pfn        = __phys_to_pfn(0x1ff00000),
        .length     = SZ_1M,
        .type       = MT_MEMORY,
    },
#endif

#ifdef CONFIG_AM_IPTV_SECURITY
    {
        .virtual    = PAGE_ALIGN(0xdfe00000),
        .pfn        = __phys_to_pfn(0x9fe00000),
        .length     = SZ_1M,
        .type       = MT_MEMORY,
    },
#endif

};

static __init void m1_map_io(void)
{
	meson_map_io();
	iotable_init(meson_video_mem_desc, ARRAY_SIZE(meson_video_mem_desc));
}

static __init void m1_irq_init(void)
{
	meson_init_irq();
}

static __init void m1_fixup(struct machine_desc *mach, struct tag *tag, char **cmdline, struct meminfo *m)
{
    struct membank *pbank;
    unsigned size;
    
    m->nr_banks = 0;
    pbank=&m->bank[m->nr_banks];
    pbank->start = PAGE_ALIGN(PHYS_MEM_START);
    pbank->size  = SZ_64M & PAGE_MASK;
    pbank->node  = PHYS_TO_NID(PHYS_MEM_START);
    m->nr_banks++;    
    pbank=&m->bank[m->nr_banks];
    pbank->start = PAGE_ALIGN(RESERVED_MEM_END+1);
   size = PHYS_MEM_END-RESERVED_MEM_END;
#ifdef CONFIG_AML_SUSPEND
    pbank->size  = (PHYS_MEM_END-RESERVED_MEM_END-SZ_1M) & PAGE_MASK;
#else
    pbank->size  = (PHYS_MEM_END-RESERVED_MEM_END) & PAGE_MASK;
#endif
#ifdef CONFIG_AM_IPTV_SECURITY
	size -= SZ_1M;
#endif
    pbank->size  = size & PAGE_MASK;

    pbank->node  = PHYS_TO_NID(RESERVED_MEM_END+1);
    m->nr_banks++;
}

MACHINE_START(MESON3_8726M_SKT, "AMLOGIC MESON3 8726M SKT SH")
    .phys_io        = MESON_PERIPHS1_PHYS_BASE,
    .io_pg_offst    = (MESON_PERIPHS1_PHYS_BASE >> 18) & 0xfffc,
    .boot_params    = BOOT_PARAMS_OFFSET,
    .map_io         = m1_map_io,
    .init_irq       = m1_irq_init,
    .timer          = &meson_sys_timer,
    .init_machine   = m1_init_machine,
    .fixup          = m1_fixup,
    .video_start    = RESERVED_MEM_START,
    .video_end      = RESERVED_MEM_END,
MACHINE_END