/*****************************************************************************
 *  Module for Microchip Graphics Library
 *  LCD controller driver
 *  Renesas R61509V
 *****************************************************************************
 * FileName:        R61509V.c
 * Dependencies:    Graphics.h
 * Processor:       PIC24, PIC32
 * Compiler:        MPLAB C30, MPLAB C32
 * Linker:          MPLAB LINK30, MPLAB LINK32
 * Company:         VirtualFab
 *
 * Software License Agreement
 *
 * Copyright (c) 2010-2013 VirtualFab  All rights reserved.
 * VirtualFab licenses to you the right to use, modify, copy and distribute
 * Software only when embedded on a Microchip microcontroller or digital
 * signal controller, which is integrated into your product or third party
 * product (pursuant to the sublicense terms in the accompanying license
 * agreement).
 *
 * You should refer to the license agreement accompanying this Software
 * for additional information regarding your rights and obligations.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED ?AS IS? WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
 * OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR
 * PURPOSE. IN NO EVENT SHALL VIRTUALFAB OR ITS LICENSORS BE LIABLE OR
 * OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION,
 * BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT
 * DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL,
 * INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA,
 * COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY
 * CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF),
 * OR OTHER SIMILAR COSTS.
 *
 * Author               Date        Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * VirtualFab           2010/12/31	Version 1.0 release
 * VirtualFab           2011/06/09	Isolated from old drv...001 and
 *                                  Window address implementation
 * VirtualFab           2011/07/15  Implementation of TRANSPARENT_COLOR
 * VirtualFab           2013/02/10  Integration for VGDD MplabX Wizard
 *****************************************************************************/
#include "Compiler.h"
#include "Graphics/Graphics.h"
#include "Graphics/gfxpmp.h"
#include "R61509V.h"

// Color
GFX_COLOR _color;
#ifdef USE_TRANSPARENT_COLOR
GFX_COLOR _colorTransparent;
SHORT _colorTransparentEnable;
#endif

// Clipping region control
SHORT _clipRgn;

// Clipping region borders
SHORT _clipLeft;
SHORT _clipTop;
SHORT _clipRight;
SHORT _clipBottom;

BYTE DeviceCode1, DeviceCode2;

/////////////////////// LOCAL FUNCTIONS PROTOTYPES ////////////////////////////
void SetReg(WORD index, WORD value);
void PutImage1BPP(SHORT left, SHORT top, FLASH_BYTE *bitmap, BYTE stretch);
void PutImage4BPP(SHORT left, SHORT top, FLASH_BYTE *bitmap, BYTE stretch);
void PutImage8BPP(SHORT left, SHORT top, FLASH_BYTE *bitmap, BYTE stretch);
void PutImage16BPP(SHORT left, SHORT top, FLASH_BYTE *bitmap, BYTE stretch);

void PutImage1BPPExt(SHORT left, SHORT top, void *bitmap, BYTE stretch);
void PutImage4BPPExt(SHORT left, SHORT top, void *bitmap, BYTE stretch);
void PutImage8BPPExt(SHORT left, SHORT top, void *bitmap, BYTE stretch);
void PutImage16BPPExt(SHORT left, SHORT top, void *bitmap, BYTE stretch);

/*********************************************************************
* Function: IsDeviceBusy()
*
* Overview: Returns non-zero if LCD controller is busy
*           (previous drawing operation is not completed).
*
* PreCondition: none
*
* Input: none
*
* Output: Busy status.
*
* Side Effects: none
*
********************************************************************/
WORD IsDeviceBusy(void) {
    return (0);
}


/*********************************************************************
 * Macro: DWORD CalcAddressXY(SHORT x, SHORT y)
 *
 * PreCondition: none
 *
 * Input: x,y - pixel coordinates
 *
 * Output: Address in display memory
 *
 * Side Effects: none
 *
 * Overview: Calculates Address based on DISP_ORIENTATION
 *
 * Note: none
 *
 ********************************************************************/
#if (DISP_ORIENTATION == 0)
    #define CalcAddressXY(x,y) ((DWORD) LINE_MEM_PITCH * (y) + (x))
#elif (DISP_ORIENTATION == 90)
    #define CalcAddressXY(x,y) ((DWORD) LINE_MEM_PITCH * (x) + (GetMaxY() - (y)))
#elif (DISP_ORIENTATION == 180)
    #define CalcAddressXY(x,y) ((DWORD) LINE_MEM_PITCH * (GetMaxY() - (y)) + (GetMaxX() -(x)))
#elif(DISP_ORIENTATION == 270)
    #define CalcAddressXY(x,y) ((DWORD) LINE_MEM_PITCH * (GetMaxX() - (x)) + (y))
#endif

/*********************************************************************
 * Macro:  WritePixel(color)
 *
 * PreCondition: none
 *
 * Input: color
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: writes pixel at the current address
 *
 * Note: chip select should be enabled
 *
 ********************************************************************/
#ifdef USE_16BIT_PMP
#define WritePixel(color)	DeviceWrite(color)
#else
#define WritePixel(color)	{ DeviceWrite(((WORD_VAL)color).v[1]); DeviceWrite(((WORD_VAL)color).v[0]);}
#endif

/*********************************************************************
 * Macros:  SetAddress(addr)
 *
 * Overview: Writes address pointer.
 *
 * PreCondition: none
 *
 * Input: add0 -  32-bit address.
 *
 * Output: none
 *
 * Side Effects: none
 *
 ********************************************************************/
#ifdef USE_16BIT_PMP
#define SetAddress(addr)                    \
	{\
        DisplaySetCommand();                     \
        DeviceWrite(0x0200);                     \
	    DisplaySetData();                        \
        DeviceWrite((WORD) addr & 0x00ff);       \
	    DisplaySetCommand();                     \
        DeviceWrite(0x0201);                     \
	    DisplaySetData();                        \
        DeviceWrite((WORD) ((DWORD) addr >> 8)); \
	    DisplaySetCommand();                     \
        DeviceWrite(0x0202);                     \
	    DisplaySetData();                        \
	}
#else
#define SetAddress(addr)                    \
	{\
	    DisplaySetCommand();                       \
        DeviceWrite(0x02);                         \
        DeviceWrite(0x00);                         \
        DisplaySetData();                          \
	    DeviceWrite(0);                            \
        DeviceWrite(((DWORD_VAL) (DWORD) addr).v[0]);\
        DisplaySetCommand();                       \
        DeviceWrite(0x02);                         \
        DeviceWrite(0x01);                         \
        DisplaySetData();                          \
        DeviceWrite(((DWORD_VAL) (DWORD) addr).v[2]);\
        DeviceWrite(((DWORD_VAL) (DWORD) addr).v[1]);\
        DisplaySetCommand();                       \
        DeviceWrite(0x02);                         \
        DeviceWrite(0x02);                         \
        DisplaySetData();                          \
	}
#endif

/*********************************************************************
 * Function:  void  SetReg(WORD index, WORD value)
 *
 * PreCondition: none
 *
 * Input: index - register number
 *        value - value to be set
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: sets graphics controller register
 *
 * Note: none
 *
 ********************************************************************/
void SetReg(WORD index, WORD value) {
#ifdef USE_16BIT_PMP
    DisplayEnable();
    DisplaySetCommand();
    DeviceWrite(index);
    DisplaySetData();
    DeviceWrite(value);
    DisplayDisable();
#else
    DisplayEnable();
    DisplaySetCommand();
    DeviceWrite(((WORD_VAL) index).v[1]);
    DeviceWrite(((WORD_VAL) index).v[0]);
    DisplaySetData();
    DeviceWrite(((WORD_VAL) value).v[1]);
    DeviceWrite(((WORD_VAL) value).v[0]);
    DisplayDisable();
#endif
}

void TransferSync(void) {
    DisplayEnable();
    DisplaySetCommand();
    DeviceWrite(0x00);
    DeviceWrite(0x00);
    DeviceWrite(0x00);
    DeviceWrite(0x00);
    DisplaySetData();
    DisplayDisable();
    DelayMs(15);
}

#if (DISP_ORIENTATION == 0)
#define REG3 0x1030 // 1000 0000 0011 0000 BGR=1 ID1=1 ID0=1 AM=0
#elif (DISP_ORIENTATION == 90)
#define REG3 0x1028 // 1000 0000 0010 1000 BGR=1 ID1=1 ID0=0 AM=1
#elif (DISP_ORIENTATION == 180)
#define REG3 0x1000 // 1000 0000 0000 0000 BGR=1 ID1=0 ID0=0 AM=0
#elif (DISP_ORIENTATION == 270)
#define REG3 0x1018 // 1000 0000 0001 1000 BGR=1 ID1=0 ID0=1 AM=1
#endif

void SetClipRgn(SHORT left, SHORT top, SHORT right, SHORT bottom) {
    _clipLeft = left;
    _clipTop = top;
    _clipRight = right;
    _clipBottom = bottom;
    SetClip(CLIP_ENABLE);
}

void SetClip(BYTE control) {
    _clipRgn = control;
}

#ifdef USE_WINDOWADDRESS

void DispEnableWindow(SHORT Left, SHORT Top, SHORT Right, SHORT Bottom) {
    volatile INT16 HSA, HEA, VSA, VEA, swap;
#if (DISP_ORIENTATION == 0)
    HSA = Left;
    HEA = Right;
    if (HEA > DISP_HOR_RESOLUTION)
        HEA = DISP_HOR_RESOLUTION;
    VSA = Top;
    VEA = Bottom;
#elif (DISP_ORIENTATION == 90)
    HSA = DISP_HOR_RESOLUTION - Bottom;
    HEA = DISP_HOR_RESOLUTION - Top;
    VSA = Left;
    VEA = Right;
    if (VEA > DISP_VER_RESOLUTION)
        VEA = DISP_VER_RESOLUTION;
#elif (DISP_ORIENTATION == 180)
    HSA = DISP_HOR_RESOLUTION - Right;
    HEA = DISP_HOR_RESOLUTION - Left;
    VSA = DISP_VER_RESOLUTION - Bottom;
    VEA = DISP_VER_RESOLUTION - Top;
#elif (DISP_ORIENTATION == 270)
    HSA = Top;
    HEA = Bottom;
    if (HEA > DISP_HOR_RESOLUTION)
        HEA = DISP_HOR_RESOLUTION;
    VSA = DISP_VER_RESOLUTION - Right;
    VEA = DISP_VER_RESOLUTION - Left;
#endif
    if (HSA < 0) HSA = 0;
    if (VSA < 0) VSA = 0;
    if (HSA > HEA) {
        swap = HEA;
        HEA = HSA;
        HSA = swap;
    }
    if (VSA > VEA) {
        swap = VEA;
        VEA = VSA;
        VSA = swap;
    }

    SetReg(0x0210, HSA); // HSA - Window Horizontal RAM Address Start (R210h)
    SetReg(0x0211, HEA); // HEA - Window Horizontal RAM Address End (R211h)
    SetReg(0x0212, VSA); // VSA - Window Vertical RAM Address Start (R212h)
    SetReg(0x0213, VEA); // VEA - Window Vertical RAM Address End (R213h)

    SetAddress(0x0000);
    SetReg(0x0003, REG3 | 0x0080); // Set ORG bit - Enable Window Address
    SetReg(0x0202, 0); // Write Data to GRAM
}

void DispDisableWindow(void) {
    SetAddress(0x0000);
    SetReg(0x0003, REG3 & 0xFFEF); // Reset ORG bit - Disable Window Address
    SetReg(0x0210, 0x0000); // Window Horizontal RAM Address Start
    SetReg(0x0211, DISP_HOR_RESOLUTION - 1); // 239 Window Horizontal RAM Address End
    SetReg(0x0212, 0x0000); // Window Vertical RAM Address Start
    SetReg(0x0213, DISP_VER_RESOLUTION - 1); // 399 Window Vertical RAM Address End
    SetReg(0x0202, 0); // Write Data to GRAM
}
#endif // USE_WINDOWADDRESS

/*********************************************************************
 * Function:  void ResetDevice()
 *
 * PreCondition: none
 *
 * Input: none
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: resets LCD, initializes PMP
 *
 * Note: none
 *
 ********************************************************************/
void ResetDevice(void) {
    // Set FLASH CS pin as output
    //DisplayFlashConfig();

    // Initialize the device
    DriverInterfaceInit();  // In gfxpmp.h

    DelayMs(5);

    // Power on LCD
    DisplayBacklightOn();
    DisplayBacklightConfig();

    DelayMs(2);

#if defined(GFX_USE_DISPLAY_CONTROLLER_R61509V)

    DisplayResetDisable(); // TFT_RST_LAT_BIT = 1
    DelayMs(10);
    DisplayResetEnable(); // TFT_RST_LAT_BIT = 0
    DelayMs(10);
    DisplayResetDisable(); // TFT_RST_LAT_BIT = 1
    DelayMs(10);

    // Power Supply ON Sequence

    // Transfer synchronization
    TransferSync();

    SetReg(0x0600, 0x0001); // soft reset
    DelayMs(10);
    SetReg(0x0600, 0x0000); // soft reset
    DelayMs(10);

    //Gamma set
    SetReg(0x0300, 0x0700); // Gamma Control
    SetReg(0x0301, 0x7811); // Gamma Control
    SetReg(0x0302, 0x0f05); // Gamma Control
    SetReg(0x0303, 0x0308); // Gamma Control
    SetReg(0x0304, 0x0111); // Gamma Control
    SetReg(0x0305, 0x0803); // Gamma Control
    SetReg(0x0306, 0x750f); // Gamma Control
    SetReg(0x0307, 0x1108); // Gamma Control
    SetReg(0x0308, 0x0007); // Gamma Control
    SetReg(0x0309, 0x1110); // Gamma Control

    SetReg(0x0007, 0x0001); // Display OFF
    SetReg(0x0010, 0x0015); // DIV RTN SET 21 clocks - default // Panel Interface Control 1 (R010h) //69.5Hz         0016
    DelayMs(10);
    SetReg(0x0100, 0x0730); // Power Control 1 - Full constant current e Full factor used in the step-up circuits
    DelayMs(10);
    SetReg(0x0101, 0x0237); // Power Control 2 - Default values
    DelayMs(10);
    SetReg(0x0103, 0x1000); // Power Control 4 - VDV: VCOM amplitude=VREG1OUT x 1.02 - vdv 1200 090313
    DelayMs(10);
    SetReg(0x0280, 0xb3ff); // NVM Data Write - VCOMH voltage A3FF 090313
    // Power Control3 (R102h)
    // [VRH4] [VRH3] [VRH2] [VRH1] [VRH0] 0 0 [VCMR] 1 0 [PSON] [PON] 0 0 0 0
    // 0      0      0      0      0      0 0 1      1 0 0      0     0 0 0 0   Default
    // 1      0      1      0      0      0 0 1      1 0 1      1     0 0 0 0   Mia
    // PON, PSON: Turn power supply ON
    // VRH[3:0]: Sets the factor to generate VREG1OUT.
    // 0x00 Halt (Hiz)          0x01-0x0F Setting inhibited
    // 0x10 VCIR x 1.600        0x11 VCIR x 1.625       0x12 VCIR x 1.650       0x13 VCIR x 1.675
    // 0x14 VCIR x 1.700        0x15 VCIR x 1.725       0x16 VCIR x 1.750       0x17 VCIR x 1.775
    // 0x18 VCIR x 1.800        0x19 VCIR x 1.825       0x1A VCIR x 1.850       0x1B VCIR x 1.875
    // 0x1C VCIR x 1.900        0x1D VCIR x 1.925       0x1E VCIR x 1.950       0x1F VCIR x 1.975
    SetReg(0x0102, 0xc1b0); // Power Control3 - PSON=1 PON=1 - Power set VRH
    DelayMs(100);

    SetReg(0x0001, 0x0100); // Driver Output Control - SS=1:the source driver output shift from S720 to S1
    SetReg(0x0002, 0x0100); // LCD Driving Wave Control - BC=1: BC = 1: line inversion waveform is selected
    // Entry Mode (R003h)
    // [TRI] [DFM] 0 [BGR] 0 0 0 0 [ORG] 0 [ID1] [ID0] [AM] 0 0 0
    //  0     0    0  0    0 0 0 0  0    0  1     1     0   0 0 0 Default value
    // AM  0=horizontal 1=vertical
    // ID0 1=increment or 0=decrement horizontal
    // ID1 1=increment or 0=decrement vertical
    // ORG 0=window address area origin address not moved 1=Origin address ?h00000? moved according to ID1-ID0
    // BGR 0=Write in RGB order 1=Reverse order from RGB to BGR
    // DFM 0=18bpp (R:G:B = 6:6:6) 1=16bpp (R:G:B = 5:6:5)
    // TRI (8bit)  0=16-bit in two transfers 1=18-bit in three transfers.
    // TRI (16bit) 0=16-bit in one transfer  1=18-bit in two   transfers.
    SetReg(0x0003, REG3);
    SetReg(0x0008, 0x020e); // FP BP set // 2 lines front porch - 14 lines back porch
    SetReg(0x0009, 0x0001); // Settings for non-display area
    SetReg(0x000C, 0x0000); // External Display Interface Control 1 - Internal clock operations - System interface/VSYNC interface
    SetReg(0x0090, 0x8000); // Frame Marker Control - FMARK signal is output from FMARK pin frame signal on


    SetReg(0x0210, 0x0000); // Window Horizontal RAM Address Start
    SetReg(0x0211, 0x00ef); // 239 Window Horizontal RAM Address End
    SetReg(0x0212, 0x0000); // Window Vertical RAM Address Start
    SetReg(0x0213, 0x018f); // 399 Window Vertical RAM Address End
    SetReg(0x0200, 0x0000); // RAM Address Set (Horizontal Address)
    SetReg(0x0201, 0x0000); // RAM Address Set (Vertical Address)
    SetReg(0x0400, 0x6200); // GS,NL,SCN SET // *Base Image Number of Line (R400h)
    SetReg(0x0401, 0x0001); // Base Image Display

    SetReg(0x0007, 0x0100); // Display Control 1 - BASEE = 1
    DelayMs(20);
    SetReg(0x0202, 0); // Write Data to GRAM

    //else
    //error Graphics controller is not supported.
#endif
    DelayMs(20);
}
#ifdef USE_TRANSPARENT_COLOR

/*********************************************************************
 * Function:  void TransparentColorEnable(GFX_COLOR color)
 *
 * Overview: Sets current transparent color.
 *
 * PreCondition: none
 *
 * Input: color - Color value chosen.
 *
 * Output: none
 *
 * Side Effects: none
 *
 ********************************************************************/
void TransparentColorEnable(GFX_COLOR color) {
    _colorTransparent = color;
    _colorTransparentEnable = TRANSPARENT_COLOR_ENABLE;

}
#endif

/*********************************************************************
 * Function: void SetAddressXY(SHORT x, SHORT y)
 *
 * PreCondition: none
 *
 * Input: x,y - pixel coordinates
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: Calculates Address based on DISP_ORIENTATION and Sets it doing nothing else
 *
 * Note: none
 *
 ********************************************************************/
void SetAddressXY(SHORT x, SHORT y) {
    DisplayEnable();
    SetAddress(CalcAddressXY(x, y));
}

/*********************************************************************
 * Function: void PutPixel(SHORT x, SHORT y)
 *
 * PreCondition: none
 *
 * Input: x,y - pixel coordinates
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: puts pixel
 *
 * Note: none
 *
 ********************************************************************/
void PutPixel(SHORT x, SHORT y) {
    if (_clipRgn) {
        if (x < _clipLeft)
            return;
        if (x > _clipRight)
            return;
        if (y < _clipTop)
            return;
        if (y > _clipBottom)
            return;
    }
    SetAddressXY(x, y);
    WritePixel(_color);
    DisplayDisable();
}

/*********************************************************************
 * Function: WORD ReadPixel(void)
 * PreCondition: none
 * Input:  none
 * Output: pixel color
 * Side Effects: none
 * Overview: returns pixel color at current address position
 * Note: none
 ********************************************************************/
#ifdef USE_16BIT_PMP

WORD ReadPixel(void) {
    WORD result;
    // Temporary change wait cycles for reading (250ns = 4 cycles)
#if defined(__C30__)
    PMMODEbits.WAITM = 4;
#elif defined(__PIC32MX__)
    PMMODEbits.WAITM = 8;
#else
#error Need wait states for the device
#endif
    DisplaySetData();

    // First RD cycle to move data from GRAM to Read Data Latch
    result = PMDIN1;

    while (PMMODEbits.BUSY);

    // Second RD cycle to get data from Read Data Latch
    result = PMDIN1;

    while (PMMODEbits.BUSY);

    // Disable LCD
    DisplayDisable();

    // Disable PMP
    PMCONbits.PMPEN = 1;

    // Read result
    result = PMDIN1;

    // Restore wait cycles for writing (60ns)
#if defined(__dsPIC33F__) || defined(__PIC24H__)
    PMMODEbits.WAITM = 2;
#else
    PMMODEbits.WAITM = 1;
#endif

    // Enable PMP
    PMCONbits.PMPEN = 1;

    return (result);
}

#else

/* */
WORD ReadPixel(void) {
    WORD_VAL result;

    // Temporary change wait cycles for reading (250ns = 4 cycles)
    /*#if defined(__C30__)
        PMMODEbits.WAITM = 4;
    #elif defined(__PIC32MX__)
        PMMODEbits.WAITM = 8;
    #else
        //error Need wait states for the device
    #endif*/
    SetReg(0x0202, 0); // Write Data to GRAM
    DisplayEnable();
    DisplaySetData();

    // First RD cycle to move data from GRAM to Read Data Latch
    result.v[1] = PMDIN1;

    while (PMMODEbits.BUSY);

    // Second RD cycle to move data from GRAM to Read Data Latch
    result.v[1] = PMDIN1;

    while (PMMODEbits.BUSY);

    // First RD cycle to get data from Read Data Latch
    // Read previous dummy value
    result.v[1] = PMDIN1;

    while (PMMODEbits.BUSY);

    // Second RD cycle to get data from Read Data Latch
    // Read MSB
    result.v[1] = PMDIN1;

    while (PMMODEbits.BUSY);

    // Disable LCD
    DisplayDisable();

    // Disable PMP
    PMCONbits.PMPEN = 1;

    // Read LSB
    result.v[0] = PMDIN1;

    /*    // Restore wait cycles for writing (60ns)
    #if defined(__dsPIC33F__) || defined(__PIC24H__)
        PMMODEbits.WAITM = 2;
    #else
        PMMODEbits.WAITM = 1;
    #endif*/

    // Enable PMP
    PMCONbits.PMPEN = 1;

    return (result.Val);
}
#endif // USE_16BIT_PMP

/*********************************************************************
 * Function: WORD GetPixel(SHORT x, SHORT y)
 * PreCondition: none
 * Input: x,y - pixel coordinates
 * Output: pixel color
 * Side Effects: none
 * Overview: returns pixel color at x,y position
 * Note: none
 ********************************************************************/
WORD GetPixel(SHORT x, SHORT y) {
    SetAddressXY(x, y);
    return (ReadPixel());
}

/*********************************************************************
 * Function: WORD Bar(SHORT left, SHORT top, SHORT right, SHORT bottom)
 *
 * PreCondition: none
 *
 * Input: left,top - top left corner coordinates,
 *        right,bottom - bottom right corner coordinates
 *
 * Output: For NON-Blocking configuration:
 *         - Returns 0 when device is busy and the shape is not yet completely drawn.
 *         - Returns 1 when the shape is completely drawn.
 *         For Blocking configuration:
 *         - Always return 1.
 *
 * Side Effects: none
 *
 * Overview: draws rectangle filled with current color
 *
 * Note: none
 *
 ********************************************************************/
WORD Bar(SHORT left, SHORT top, SHORT right, SHORT bottom) {
    DWORD address;
    register SHORT x, y;

#ifndef USE_NONBLOCKING_CONFIG
    while (IsDeviceBusy() != 0);

    /* Ready */
#else
    if (IsDeviceBusy() != 0)
        return (0);
#endif
    if (_clipRgn) {
        if (left < _clipLeft)
            left = _clipLeft;
        if (right > _clipRight)
            right = _clipRight;
        if (top < _clipTop)
            top = _clipTop;
        if (bottom > _clipBottom)
            bottom = _clipBottom;
    }

#if (DISP_ORIENTATION == 0)
    address = (DWORD) LINE_MEM_PITCH * top + left;
    DisplayEnable();
    for (y = top; y < bottom + 1; y++) {
        SetAddress(address);
        for (x = left; x < right + 1; x++) {
            WritePixel(_color);
        }
        address += LINE_MEM_PITCH;
    }
    DisplayDisable();

#elif (DISP_ORIENTATION == 90)  //F
    top = GetMaxY() - top;
    bottom = GetMaxY() - bottom;
    address = (DWORD) LINE_MEM_PITCH * left + top;
    DisplayEnable();
    for (y = bottom; y < top + 1; y++) {
        SetAddress(address);
        for (x = left; x <  right + 1; x++) {
            WritePixel(_color);
        }
        address -= 1;
    }
    DisplayDisable();

#elif (DISP_ORIENTATION == 180)
    top=GetMaxY()-top;
    bottom=GetMaxY()-bottom;
    left=GetMaxX()-left;
    right=GetMaxX()-right;
    address = (DWORD) LINE_MEM_PITCH * top + left;
    DisplayEnable();
    for (y = bottom; y < top + 1; y++) {
        SetAddress(address);
        for (x = right; x < left + 1; x++) {
            WritePixel(_color);
        }
        address -= LINE_MEM_PITCH;
    }
    DisplayDisable();

#elif (DISP_ORIENTATION == 270)  //F
    left = GetMaxX() - left;
    right = GetMaxX() - right;
    address = (DWORD) LINE_MEM_PITCH * left + bottom;

    DisplayEnable();
    for (y = top; y < bottom + 1; y++) {
        SetAddress(address);
        for (x = right; x < left + 1; x++) {
            WritePixel(_color);
        }
        address -= 1;
    }
    DisplayDisable();

#endif
    return (1);
}

/*********************************************************************
 * Function: void ClearDevice(void)
 *
 * PreCondition: none
 *
 * Input: none
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: clears screen with current color
 *
 * Note: none
 *
 ********************************************************************/
void ClearDevice(void) {
    DWORD counter;

    DisplayEnable();
    SetAddress(0);
    for (counter = 0; counter < (DWORD) (GetMaxX() + 1) * (GetMaxY() + 1); counter++) {
        WritePixel(_color);
    }

    DisplayDisable();
}

//#ifdef USE_DRV_PUTIMAGE

/*********************************************************************
 * Function: WORD PutImage(SHORT left, SHORT top, void* bitmap, BYTE stretch)
 *
 * PreCondition: none
 *
 * Input: left,top - left top image corner,
 *        bitmap - image pointer,
 *        stretch - image stretch factor
 *
 * Output: For NON-Blocking configuration:
 *         - Returns 0 when device is busy and the image is not yet completely drawn.
 *         - Returns 1 when the image is completely drawn.
 *         For Blocking configuration:
 *         - Always return 1.
 *
 * Side Effects: none
 *
 * Overview: outputs image starting from left,top coordinates
 *
 * Note: image must be located in flash
 *
 ********************************************************************/

/* */
WORD __attribute__((weak)) PutImagePartial(SHORT left, SHORT top, void *image, BYTE stretch, SHORT xoffset, SHORT yoffset, WORD width, WORD height) {
#if defined (USE_BITMAP_FLASH) || defined (USE_BITMAP_EXTERNAL)
    FLASH_BYTE *flashAddress;
    BYTE colorDepth;
#endif    
    WORD colorTemp;
    WORD ret = 0;

#ifndef USE_NONBLOCKING_CONFIG
    while (IsDeviceBusy() != 0);

    /* Ready */
#else
    if (IsDeviceBusy() != 0)
        return (0);
#endif
    /*#if (DISP_ORIENTATION == 90)
        top = GetMaxY() - top;
    #endif*/

    // Save current color
    colorTemp = _color;

    switch (*((SHORT *) image)) {

#ifdef USE_BITMAP_FLASH

        case FLASH:

            // Image address
            flashAddress = ((IMAGE_FLASH *) image)->address;

            // Read color depth
            colorDepth = *(flashAddress + 1);

            // Draw picture
            switch (colorDepth) {
                case 1: PutImage1BPP(left, top, flashAddress, stretch);
                    break;
                case 4: PutImage4BPP(left, top, flashAddress, stretch);
                    break;
                case 8: PutImage8BPP(left, top, flashAddress, stretch);
                    break;
                case 16: PutImage16BPP(left, top, flashAddress, stretch);
                    break;
            }
            ret = 1;
            break;
#endif
#if defined(USE_BITMAP_EXTERNAL) || defined(USE_BITMAP_SD)
        case BINBMP_ON_SDFAT:
        case EXTERNAL:

            // Get color depth
            ExternalMemoryCallback(image, 1, 1, &colorDepth);

            // Draw picture
            switch (colorDepth) {
                case 1: PutImage1BPPExt(left, top, image, stretch);
                    break;
                case 4: PutImage4BPPExt(left, top, image, stretch);
                    break;
                case 8: PutImage8BPPExt(left, top, image, stretch);
                    break;
                case 16: PutImage16BPPExt(left, top, image, stretch);
                    break;
                default: break;
            }
            ret = 1;
            break;
#endif

        default:
            break;
    }

    // Restore current color
    _color = colorTemp;
    return (ret);
}

#if defined(USE_BITMAP_FLASH)

/*********************************************************************
 * Function: void PutImage1BPP(SHORT left, SHORT top, FLASH_BYTE* bitmap, BYTE stretch)
 *
 * PreCondition: none
 *
 * Input: left,top - left top image corner,
 *        bitmap - image pointer,
 *        stretch - image stretch factor
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: outputs monochrome image starting from left,top coordinates
 *
 * Note: image must be located in flash
 *
 ********************************************************************/
void PutImage1BPP(SHORT left, SHORT top, FLASH_BYTE *bitmap, BYTE stretch) {
    register FLASH_BYTE *flashAddress;
    register FLASH_BYTE *tempFlashAddress;
#ifndef USE_WINDOWADDRESS
    DWORD address;
#endif
    BYTE temp = 0;
    WORD sizeX, sizeY;
    WORD x, y;
    BYTE stretchX, stretchY;
    WORD pallete[2];
    BYTE mask;

    // Move pointer to size information
    flashAddress = bitmap + 2;

    // Read image size
    sizeY = *((FLASH_WORD *) flashAddress);
    flashAddress += 2;
    sizeX = *((FLASH_WORD *) flashAddress);
    flashAddress += 2;
    pallete[0] = *((FLASH_WORD *) flashAddress);
    flashAddress += 2;
    pallete[1] = *((FLASH_WORD *) flashAddress);
    flashAddress += 2;

#ifdef USE_WINDOWADDRESS
    DispEnableWindow(left, top, left + sizeX * stretch - 1, top + sizeY * stretch - 1);
    SetAddress(0);
#endif

    DisplayEnable();
    for (y = 0; y < sizeY; y++) {
        tempFlashAddress = flashAddress;
        for (stretchY = 0; stretchY < stretch; stretchY++) {
            flashAddress = tempFlashAddress;
#ifndef USE_WINDOWADDRESS
            address = CalcAddressXY(left, top + y * stretch + stretchY);
            SetAddress(address);
#endif
            mask = 0;
            for (x = 0; x < sizeX; x++) {

                // Read 8 pixels from flash
                if (mask == 0) {
                    temp = *flashAddress;
                    flashAddress++;
                    mask = 0x80;
                }

                // Set color
                if (mask & temp) {
                    SetColor(pallete[1]);
                } else {
                    SetColor(pallete[0]);
                }

                // Write pixel to screen
                for (stretchX = 0; stretchX < stretch; stretchX++) {
#ifdef USE_TRANSPARENT_COLOR
                    if ((GetTransparentColor() == _color) && (GetTransparentColorStatus() == TRANSPARENT_COLOR_ENABLE)) {
#ifdef USE_WINDOWADDRESS
                        address = CalcAddressXY(x * stretch + stretchX + 1, y * stretch + stretchY);
                        SetAddress(address);
#else
                        address = CalcAddressXY(x * stretch + stretchX + 1 + left, y * stretch + stretchY + top);
                        SetAddress(address);
#endif
                    } else
#endif
                        WritePixel(_color);
                }

                // Shift to the next pixel
                mask >>= 1;
            }
        }
    }

    DisplayDisable();
#ifdef USE_WINDOWADDRESS
    DispDisableWindow();
#endif
}

/*********************************************************************
 * Function: void PutImage4BPP(SHORT left, SHORT top, FLASH_BYTE* bitmap, BYTE stretch)
 *
 * PreCondition: none
 *
 * Input: left,top - left top image corner, bitmap - image pointer,
 *        stretch - image stretch factor
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: outputs 16 color image starting from left,top coordinates
 *
 * Note: image must be located in flash
 *
 ********************************************************************/
void PutImage4BPP(SHORT left, SHORT top, FLASH_BYTE *bitmap, BYTE stretch) {
    register FLASH_BYTE *flashAddress;
    register FLASH_BYTE *tempFlashAddress;
#ifndef USE_WINDOWADDRESS
    DWORD address;
#endif
    WORD sizeX, sizeY;
    register WORD x, y;
    BYTE temp = 0;
    register BYTE stretchX, stretchY;
    WORD pallete[16];
    WORD counter;

    // Move pointer to size information
    flashAddress = bitmap + 2;

    // Read image size
    sizeY = *((FLASH_WORD *) flashAddress);
    flashAddress += 2;
    sizeX = *((FLASH_WORD *) flashAddress);
    flashAddress += 2;

    // Read pallete
    for (counter = 0; counter < 16; counter++) {
        pallete[counter] = *((FLASH_WORD *) flashAddress);
        flashAddress += 2;
    }

#ifdef USE_WINDOWADDRESS
    DispEnableWindow(left, top, left + sizeX * stretch - 1, top + sizeY * stretch - 1);
    SetAddress(0);
#endif

    DisplayEnable();
    for (y = 0; y < sizeY; y++) {
        tempFlashAddress = flashAddress;
        for (stretchY = 0; stretchY < stretch; stretchY++) {
            flashAddress = tempFlashAddress;

            // Set start address
#ifndef USE_WINDOWADDRESS
            address = CalcAddressXY(left, top + y * stretch + stretchY);
            SetAddress(address);
#endif
            for (x = 0; x < sizeX; x++) {
                // Read 2 pixels from flash
                if (x & 0x0001) {
                    // second pixel in byte
                    SetColor(pallete[temp >> 4]);
                } else {
                    temp = *flashAddress;
                    flashAddress++;

                    // first pixel in byte
                    SetColor(pallete[temp & 0x0f]);
                }

                // Write pixel to screen
                for (stretchX = 0; stretchX < stretch; stretchX++) {
#ifdef USE_TRANSPARENT_COLOR
                    if ((GetTransparentColor() == _color) && (GetTransparentColorStatus() == TRANSPARENT_COLOR_ENABLE)) {
#ifdef USE_WINDOWADDRESS
                        address = CalcAddressXY(x * stretch + stretchX + 1, y * stretch + stretchY);
                        SetAddress(address);
#else
                        address = CalcAddressXY(x * stretch + stretchX + 1 + left, y * stretch + stretchY + top);
                        SetAddress(address);
#endif
                    } else
#endif
                        WritePixel(_color);
                }

                // Shift to the next pixel
                //temp >>= 4;
            }
        }
    }

    DisplayDisable();
#ifdef USE_WINDOWADDRESS
    DispDisableWindow();
#endif
}

/*********************************************************************
 * Function: void PutImage8BPP(SHORT left, SHORT top, FLASH_BYTE* bitmap, BYTE stretch)
 *
 * PreCondition: none
 *
 * Input: left,top - left top image corner, bitmap - image pointer,
 *        stretch - image stretch factor
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: outputs 256 color image starting from left,top coordinates
 *
 * Note: image must be located in flash
 *
 ********************************************************************/
void PutImage8BPP(SHORT left, SHORT top, FLASH_BYTE *bitmap, BYTE stretch) {
    register FLASH_BYTE *flashAddress;
    register FLASH_BYTE *tempFlashAddress;
#ifndef USE_WINDOWADDRESS
    DWORD address;
#endif
    WORD sizeX, sizeY;
    WORD x, y;
    BYTE PaletteIndex;
    BYTE stretchX, stretchY;
    WORD pallete[256];
    WORD counter;

    // Move pointer to size information
    flashAddress = bitmap + 2;

    // Read image size
    sizeY = *((FLASH_WORD *) flashAddress);
    flashAddress += 2;
    sizeX = *((FLASH_WORD *) flashAddress);
    flashAddress += 2;

    // Read pallete
    for (counter = 0; counter < 256; counter++) {
        pallete[counter] = *((FLASH_WORD *) flashAddress);
        flashAddress += 2;
    }

#ifdef USE_WINDOWADDRESS
    DispEnableWindow(left, top, left + sizeX * stretch - 1, top + sizeY * stretch - 1);
    SetAddress(0);
#endif

    DisplayEnable();
    for (y = 0; y < sizeY; y++) {
        tempFlashAddress = flashAddress;
        for (stretchY = 0; stretchY < stretch; stretchY++) {
            flashAddress = tempFlashAddress;

            // Set start address
#ifndef USE_WINDOWADDRESS
            address = CalcAddressXY(left, top + y * stretch + stretchY);
            SetAddress(address);
#endif
            for (x = 0; x < sizeX; x++) {

                // Read pixels from flash
                PaletteIndex = *flashAddress;
                flashAddress++;

                // Set color
                SetColor(pallete[PaletteIndex]);

                // Write pixel to screen
                for (stretchX = 0; stretchX < stretch; stretchX++) {
#ifdef USE_TRANSPARENT_COLOR
                    if ((GetTransparentColor() == _color) && (GetTransparentColorStatus() == TRANSPARENT_COLOR_ENABLE)) {
#ifdef USE_WINDOWADDRESS
                        address = CalcAddressXY(x * stretch + stretchX + 1, y * stretch + stretchY);
                        SetAddress(address);
#else
                        address = CalcAddressXY(x * stretch + stretchX + 1 + left, y * stretch + stretchY + top);
                        SetAddress(address);
#endif
                    } else
#endif
                        WritePixel(_color);
                }
            }
        }
    }
    DisplayDisable();
#ifdef USE_WINDOWADDRESS
    DispDisableWindow();
#endif
}

/*********************************************************************
 * Function: void PutImage16BPP(SHORT left, SHORT top, FLASH_BYTE* bitmap, BYTE stretch)
 *
 * PreCondition: none
 *
 * Input: left,top - left top image corner, bitmap - image pointer,
 *        stretch - image stretch factor
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: outputs hicolor image starting from left,top coordinates
 *
 * Note: image must be located in flash
 *
 ********************************************************************/
void PutImage16BPP(SHORT left, SHORT top, FLASH_BYTE *bitmap, BYTE stretch) {
    register FLASH_WORD *flashAddress;
    register FLASH_WORD *tempFlashAddress;
#ifndef USE_WINDOWADDRESS
    DWORD address;
#endif
    WORD sizeX, sizeY;
    register WORD x, y;
    WORD PicPixelColor;
    register BYTE stretchX, stretchY;

    // Move pointer to size information
    flashAddress = (FLASH_WORD *) bitmap + 1;

    // Read image size
    sizeY = *flashAddress;
    flashAddress++;
    sizeX = *flashAddress;
    flashAddress++;

#ifdef USE_WINDOWADDRESS
    DispEnableWindow(left, top, left + sizeX * stretch - 1, top + sizeY * stretch - 1);
    SetAddress(0);
#endif

    DisplayEnable();
    for (y = 0; y < sizeY; y++) {
        tempFlashAddress = flashAddress;
        for (stretchY = 0; stretchY < stretch; stretchY++) {
            flashAddress = tempFlashAddress;

            // Set start address
#ifndef USE_WINDOWADDRESS
            address = CalcAddressXY(left, top + y * stretch + stretchY);
            SetAddress(address);
#endif

            for (x = 0; x < sizeX; x++) {

                // Read pixels from flash
                PicPixelColor = *flashAddress;
                flashAddress++;

                // Write pixel to screen
                for (stretchX = 0; stretchX < stretch; stretchX++) {
#ifdef USE_TRANSPARENT_COLOR
                    if ((GetTransparentColor() == PicPixelColor) && (GetTransparentColorStatus() == TRANSPARENT_COLOR_ENABLE)) {
#ifdef USE_WINDOWADDRESS
                        address = CalcAddressXY(x * stretch + stretchX + 1, y * stretch + stretchY);
                        SetAddress(address);
#else
                        address = CalcAddressXY(x * stretch + stretchX + 1 + left, y * stretch + stretchY + top);
                        SetAddress(address);
#endif
                    } else
#endif
                        WritePixel(PicPixelColor);
                }
            }
        }
    }

    DisplayDisable();
#ifdef USE_WINDOWADDRESS
    DispDisableWindow();
#endif
}

#endif //USE_BITMAP_FLASH

#if defined(USE_BITMAP_EXTERNAL) || defined(USE_BITMAP_SD)

/*********************************************************************
 * Function: void PutImage1BPPExt(SHORT left, SHORT top, void* bitmap, BYTE stretch)
 *
 * PreCondition: none
 *
 * Input: left,top - left top image corner, bitmap - image pointer,
 *        stretch - image stretch factor
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: outputs monochrome image starting from left,top coordinates
 *
 * Note: image must be located in external memory
 *
 ********************************************************************/
void PutImage1BPPExt(SHORT left, SHORT top, void *bitmap, BYTE stretch) {
    register DWORD memOffset;
#ifndef USE_WINDOWADDRESS
    DWORD address;
#endif
    BITMAP_HEADER bmp;
    WORD pallete[2];
    BYTE lineBuffer[((GetMaxX() + 1) / 8) + 1];
    BYTE *pData;
    SHORT byteWidth;

    BYTE temp = 0;
    BYTE mask;
    WORD sizeX, sizeY;
    WORD x, y;
    BYTE stretchX, stretchY;

    // Get bitmap header
    ExternalMemoryCallback(bitmap, 0, sizeof (BITMAP_HEADER), &bmp);

    // Get pallete (2 entries)
    ExternalMemoryCallback(bitmap, sizeof (BITMAP_HEADER), 2 * sizeof (WORD), pallete);

    // Set offset to the image data
    memOffset = sizeof (BITMAP_HEADER) + 2 * sizeof (WORD);

    // Line width in bytes
    byteWidth = bmp.width >> 3;
    if (bmp.width & 0x0007)
        byteWidth++;

    // Get size
    sizeX = bmp.width;
    sizeY = bmp.height;

#ifdef USE_WINDOWADDRESS
    DispEnableWindow(left, top, left + sizeX * stretch - 1, top + sizeY * stretch - 1);
    SetAddress(0);
#endif

    for (y = 0; y < sizeY; y++) {

        // Get line
        ExternalMemoryCallback(bitmap, memOffset, byteWidth, lineBuffer);
        memOffset += byteWidth;
        DisplayEnable();
        for (stretchY = 0; stretchY < stretch; stretchY++) {
            pData = lineBuffer;
#ifndef USE_WINDOWADDRESS
            address = CalcAddressXY(left, top + y * stretch + stretchY);
            SetAddress(address);
#endif
            mask = 0;
            for (x = 0; x < sizeX; x++) {

                // Read 8 pixels from flash
                if (mask == 0) {
                    temp = *pData++;
                    mask = 0x80;
                }

                // Set color
                if (mask & temp) {
                    SetColor(pallete[1]);
                } else {
                    SetColor(pallete[0]);
                }

                // Write pixel to screen
                for (stretchX = 0; stretchX < stretch; stretchX++) {
#ifdef USE_TRANSPARENT_COLOR
                    if ((GetTransparentColor() == _color) && (GetTransparentColorStatus() == TRANSPARENT_COLOR_ENABLE)) {
#ifdef USE_WINDOWADDRESS
                        address = CalcAddressXY(x * stretch + stretchX + 1, y * stretch + stretchY);
                        SetAddress(address);
#else
                        address = CalcAddressXY(x * stretch + stretchX + 1 + left, y * stretch + stretchY + top);
                        SetAddress(address);
#endif
                    } else
#endif
                        WritePixel(_color);
                }

                // Shift to the next pixel
                mask >>= 1;
            }
        }
        DisplayDisable();
    }
#ifdef USE_WINDOWADDRESS
    DispDisableWindow();
#endif
}

/*********************************************************************
 * Function: void PutImage4BPPExt(SHORT left, SHORT top, void* bitmap, BYTE stretch)
 *
 * PreCondition: none
 *
 * Input: left,top - left top image corner, bitmap - image pointer,
 *        stretch - image stretch factor
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: outputs monochrome image starting from left,top coordinates
 *
 * Note: image must be located in external memory
 *
 ********************************************************************/
void PutImage4BPPExt(SHORT left, SHORT top, void *bitmap, BYTE stretch) {
    register DWORD memOffset;
#ifndef USE_WINDOWADDRESS
    DWORD address;
#endif
    BITMAP_HEADER bmp;
    WORD pallete[16];
    BYTE lineBuffer[((GetMaxX() + 1) / 2) + 1];
    BYTE *pData;
    SHORT byteWidth;

    BYTE temp = 0;
    WORD sizeX, sizeY;
    WORD x, y;
    BYTE stretchX, stretchY;

    // Get bitmap header
    ExternalMemoryCallback(bitmap, 0, sizeof (BITMAP_HEADER), &bmp);

    // Get pallete (16 entries)
    ExternalMemoryCallback(bitmap, sizeof (BITMAP_HEADER), 16 * sizeof (WORD), pallete);

    // Set offset to the image data
    memOffset = sizeof (BITMAP_HEADER) + 16 * sizeof (WORD);

    // Line width in bytes
    byteWidth = bmp.width >> 1;
    if (bmp.width & 0x0001)
        byteWidth++;

    // Get size
    sizeX = bmp.width;
    sizeY = bmp.height;

#ifdef USE_WINDOWADDRESS
    DispEnableWindow(left, top, left + sizeX * stretch - 1, top + sizeY * stretch - 1);
#endif
    SetAddress(0);

    for (y = 0; y < sizeY; y++) {
        // Get line
        ExternalMemoryCallback(bitmap, memOffset, byteWidth, lineBuffer);
        memOffset += byteWidth;
        DisplayEnable();
        for (stretchY = 0; stretchY < stretch; stretchY++) {
            pData = lineBuffer;
#ifndef USE_WINDOWADDRESS
            address = CalcAddressXY(left, top + y * stretch + stretchY);
            SetAddress(address);
#endif
            for (x = 0; x < sizeX; x++) {
                // Read 2 pixels from flash
                if (x & 0x0001) {
                    // second pixel in byte
                    SetColor(pallete[temp >> 4]);
                } else {
                    temp = *pData++;
                    // first pixel in byte
                    SetColor(pallete[temp & 0x0f]);
                }
                // Write pixel to screen
                for (stretchX = 0; stretchX < stretch; stretchX++) {
#ifdef USE_TRANSPARENT_COLOR
                    if ((GetTransparentColor() == _color) && (GetTransparentColorStatus() == TRANSPARENT_COLOR_ENABLE)) {
#ifdef USE_WINDOWADDRESS
                        address = CalcAddressXY(x * stretch + stretchX + 1, y * stretch + stretchY);
                        SetAddress(address);
#else
                        address = CalcAddressXY(x * stretch + stretchX + 1 + left, y * stretch + stretchY + top);
                        SetAddress(address);
#endif
                    } else
#endif
                        WritePixel(_color);
                }
            }
        }
        DisplayDisable();
    }
#ifdef USE_WINDOWADDRESS
    DispDisableWindow();
#endif
}

/*********************************************************************
 * Function: void PutImage8BPPExt(SHORT left, SHORT top, void* bitmap, BYTE stretch)
 *
 * PreCondition: none
 *
 * Input: left,top - left top image corner, bitmap - image pointer,
 *        stretch - image stretch factor
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: outputs monochrome image starting from left,top coordinates
 *
 * Note: image must be located in external memory
 *
 ********************************************************************/
void PutImage8BPPExt(SHORT left, SHORT top, void *bitmap, BYTE stretch) {
    register DWORD memOffset;
#ifndef USE_WINDOWADDRESS
    DWORD address;
#endif
    BITMAP_HEADER bmp;
    WORD pallete[256];
    BYTE lineBuffer[(GetMaxX() + 1)];
    BYTE *pData;

    BYTE temp;
    WORD sizeX, sizeY; // TODO: rimuovere volatile
    WORD x, y;
    BYTE stretchX, stretchY;

    // Get bitmap header
    ExternalMemoryCallback(bitmap, 0, sizeof (BITMAP_HEADER), &bmp);
    // Get pallete (256 entries)
    ExternalMemoryCallback(bitmap, sizeof (BITMAP_HEADER), 256 * sizeof (WORD), pallete);
    // Set offset to the image data
    memOffset = sizeof (BITMAP_HEADER) + 256 * sizeof (WORD);
    // Get size
    sizeX = bmp.width;
    sizeY = bmp.height;
#ifdef USE_WINDOWADDRESS
    DispEnableWindow(left, top, left + sizeX * stretch, top + sizeY * stretch);
#endif
    SetAddress(0);
    for (y = 0; y < sizeY; y++) {
        // Get line
        ExternalMemoryCallback(bitmap, memOffset, sizeX, lineBuffer);
        memOffset += sizeX;
        DisplayEnable();
        for (stretchY = 0; stretchY < stretch; stretchY++) {
            pData = lineBuffer;
#ifndef USE_WINDOWADDRESS
            // Set start address
            address = CalcAddressXY(left, top + y * stretch + stretchY);
            SetAddress(address);
#endif
            for (x = 0; x < sizeX; x++) {
                temp = *pData++;
                SetColor(pallete[temp]);
                
                // Write pixel to screen
                for (stretchX = 0; stretchX < stretch; stretchX++) {
#ifdef USE_TRANSPARENT_COLOR
                    if ((GetTransparentColor() == _color) && (GetTransparentColorStatus() == TRANSPARENT_COLOR_ENABLE)) {
#ifdef USE_WINDOWADDRESS
                        address = CalcAddressXY(x * stretch + stretchX + 1, y * stretch + stretchY);
                        SetAddress(address);
#else
                        address = CalcAddressXY(x * stretch + stretchX + 1 + left, y * stretch + stretchY + top);
                        SetAddress(address);
#endif
                    } else
#endif
                        WritePixel(_color);
                }
            }
        }
        DisplayDisable();
    }
#ifdef USE_WINDOWADDRESS
    DispDisableWindow();
#endif
}

/*********************************************************************
 * Function: void PutImage16BPPExt(SHORT left, SHORT top, void* bitmap, BYTE stretch)
 *
 * PreCondition: none
 *
 * Input: left,top - left top image corner, bitmap - image pointer,
 *        stretch - image stretch factor
 *
 * Output: none
 *
 * Side Effects: none
 *
 * Overview: outputs monochrome image starting from left,top coordinates
 *
 * Note: image must be located in external memory
 *
 ********************************************************************/
void PutImage16BPPExt(SHORT left, SHORT top, void *bitmap, BYTE stretch) {
    register DWORD memOffset;
#ifndef USE_WINDOWADDRESS
    volatile DWORD address;
#endif
    BITMAP_HEADER bmp;
    WORD lineBuffer[(GetMaxX() + 1)];
    WORD *pData;
    WORD byteWidth;

    WORD temp;
    WORD sizeX, sizeY;
    WORD x, y;
    BYTE stretchX, stretchY;

    // Get bitmap header
    ExternalMemoryCallback(bitmap, 0, sizeof (BITMAP_HEADER), &bmp);
    // Set offset to the image data
    memOffset = sizeof (BITMAP_HEADER);
    // Get size
    sizeX = bmp.width;
    sizeY = bmp.height;
#ifdef USE_WINDOWADDRESS
    DispEnableWindow(left, top, left + sizeX * stretch - 1, top + sizeY * stretch - 1);
#endif
    SetAddress(0);
    byteWidth = sizeX << 1;
    for (y = 0; y < sizeY; y++) {
        // Get line
        ExternalMemoryCallback(bitmap, memOffset, byteWidth, lineBuffer);
        memOffset += byteWidth;
        DisplayEnable();
        for (stretchY = 0; stretchY < stretch; stretchY++) {
            pData = lineBuffer;
#ifndef USE_WINDOWADDRESS
            address = CalcAddressXY(left, top + y * stretch + stretchY);
            SetAddress(address);
#endif
            for (x = 0; x < sizeX; x++) {
                temp = *pData++;
                SetColor(temp);
                // Write pixel to screen
                for (stretchX = 0; stretchX < stretch; stretchX++) {
#ifdef USE_TRANSPARENT_COLOR
                    if ((GetTransparentColor() == temp) && (GetTransparentColorStatus() == TRANSPARENT_COLOR_ENABLE)) {
#ifdef USE_WINDOWADDRESS
                        address = CalcAddressXY(x * stretch + stretchX + 1, y * stretch + stretchY);
                        SetAddress(address);
#else
                        address = CalcAddressXY(x * stretch + stretchX + 1 + left, y * stretch + stretchY + top);
                        SetAddress(address);
#endif
                    } else
#endif
                        WritePixel(_color);
                }
            }
        }
        DisplayDisable();
    }
#ifdef USE_WINDOWADDRESS
    DispDisableWindow();
#endif
}
#endif // USE_BITMAP_EXTERNAL
//#endif // USE_DRV_PUTIMAGE
