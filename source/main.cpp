#include <curl/curl.h> //curl
#include <3ds.h>       //libctru

#include <stdio.h>
#include <string>
#include "network.hpp"
#include "ui.hpp"
#include "serial.hpp"

DrawContext ctx;

#ifndef SERVER_ADDRESS
#define SERVER_ADDRESS "http://soap.gorgerush.net:9023/submit"
#endif

void enter(char* inout, size_t len) {
    SwkbdState swkbd;
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, len);
    swkbdSetInitialText(&swkbd, inout);
    swkbdInputText(&swkbd, inout, len+1);
    return;
}

int main(int argc, char** argv) {
    bool soapfinished = false;
    C2D_SpriteSheet sheet = NULL;
    gfxInitDefault();
    romfsInit();
    

    int result;
    char discordtag[33] = "";
    char address[52] = "http://soap.gorgerush.net:9023/submit";
    initContext(&ctx);
    initColors(&ctx);
    int menustate = 0;
    std::string finaltext;
    std::string outputData;
    bool usenandessential = false;

    if (!initSocket()) {
        goto fail;
    }

    result = initUI();
    if (result) {
        printf("UI failed to initialize: returned %d", result);
        goto fail;
    }
    
    setNANDEssentialSerial();
    setSDEssentialSerial();
    setTWLNSerial();
    setSecinfoSerial();


    sheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    C2D_Sprite bhlogo, soap;
    C2D_SpriteFromSheet(&bhlogo, sheet, 0);
    C2D_SpriteFromSheet(&soap, sheet, 1);

    C2D_SpriteSetPos(&bhlogo, 10, 10);
    C2D_SpriteSetScale(&bhlogo, 0.4, 0.4);
    C2D_SpriteSetPos(&soap, SCREEN_WIDTH_BOTTOM/2, SCREEN_HEIGHT/2);
    C2D_SpriteSetScale(&soap, 0.7, 0.7);
    C2D_SpriteSetCenter(&bhlogo, 0.0, 0.0);
    C2D_SpriteSetCenter(&soap, 0.5, 0.5);

    while (aptMainLoop()) {
        hidScanInput();
        touchPosition touch;
        hidTouchRead(&touch);
        u32 kDown = hidKeysHeld();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(ctx.top, ctx.clrBgDark);
        C2D_TargetClear(ctx.bottom, ctx.clrBgDark);
        C2D_SceneBegin(ctx.bottom);
        C2D_DrawSprite(&soap);
        C2D_SceneBegin(ctx.top);
        drawText(115, 10, 0, 0.7, ctx.clrWhite, 0, "Bluehax Transporter");
        drawText(115, 55, 0, 0.4, ctx.clrWhite, 0, "Credits to gruetzig for original essentialsubmit application");
        C2D_DrawSprite(&bhlogo);
        drawText(160,  80, 0, 0.4, ctx.clrWhite, 0, "SD essential.exefs serial:");
        drawText(160,  90, 0, 0.4, ctx.clrWhite, 0, "NAND essential.exefs serial:");
        drawText(160, 100, 0, 0.4, ctx.clrWhite, 0, "SecureInfo serial:");
        drawText(160, 110, 0, 0.4, ctx.clrWhite, 0, "inspect.log serial:");
        drawText(320,  80, 0, 0.4, ctx.clrWhite, 0, "%s", getSDEssentialSerial());
        drawText(320,  90, 0, 0.4, ctx.clrWhite, 0, "%s", getNANDEssentialSerial());
        drawText(320, 100, 0, 0.4, ctx.clrWhite, 0, "%s", getSecinfoSerial());
        drawText(320, 110, 0, 0.4, ctx.clrWhite, 0, "%s", getTWLNSerial());
        
        switch(menustate) {
            case 0:
                drawText(20, 210, 0, 0.5, ctx.clrWhite, 0, "Server address: %s", address);
                if (strlen(address) > 0 && strlen(discordtag) > 0) {
                    drawText(20, 180, 0, 0.5, ctx.clrWhite, 0, "Your Discord username: %s", discordtag);
                    drawText(SCREEN_WIDTH_TOP/2, SCREEN_HEIGHT/2+20, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Press on the soap to submit");
                } else {
                    drawText(SCREEN_WIDTH_TOP/2, SCREEN_HEIGHT/2+20, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Press Y to enter your Discord username");
                }
                break;
            case 1:
                drawText(SCREEN_WIDTH_TOP/2, SCREEN_HEIGHT*3/4, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Submitting...");
                break;
            case 2:
                if (!soapfinished) {
                    drawText(SCREEN_WIDTH_TOP/2, SCREEN_HEIGHT*3/4, 0, 0.5, ctx.clrRed, C2D_AlignCenter, finaltext.c_str());
                } else {
                    drawText(SCREEN_WIDTH_TOP/2, SCREEN_HEIGHT*3/4, 0, 0.5, ctx.clrWhite, C2D_AlignCenter, finaltext.c_str());
                }
                break;

        }
        C3D_FrameEnd(0);
        switch(menustate) {
            case 0:
                if ((kDown & KEY_X) && (kDown & KEY_DDOWN)) {
                    enter(address, 51);
                }
                if ((kDown & KEY_L) && (kDown & KEY_DUP)) {
                    sprintf(discordtag, getNANDEssentialSerial());
                }
                if (kDown & KEY_Y) {
                    enter(discordtag, 32);
                }
                if (strlen(address) > 0 && strlen(discordtag) > 0 && (kDown & KEY_TOUCH || kDown & KEY_A)) {
                    menustate++;
                }
                if (kDown & KEY_START) {
                    goto deinit;
                }
                break;
            case 2:
                if (kDown & KEY_START) {
                    goto deinit;
                }
                break;
            case 1:
                if (getSDEssentialSerial()[0] == '\0') {
                    if (getNANDEssentialSerial()[0] == '\0') {
                        finaltext = "essential.exefs not found";
                        menustate++;
                        break;
                    }
                    usenandessential = true;
                }
                initcurl();
                initform(); 
                discordhandleentry(discordtag);
                if (usenandessential) {
                    essentialdataentry();
                } else {
                    fileentry("sdmc:/gm9/out/essential.exefs");
                }
                serialentry("sd", getSDEssentialSerial());
                serialentry("nand", getNANDEssentialSerial());
                serialentry("twln", getTWLNSerial());
                serialentry("secinfo", getSecinfoSerial());
                CURLcode res = submittourl(address, &outputData);
                if(res != CURLE_OK) {
                    finaltext = std::string("Submission failed: ") + curl_easy_strerror(res) + "\n";
                } else {
                    long http_code = gethttpcode();
                    if(http_code == 200) {
                        soapfinished = true;
                    }
                    finaltext = outputData;
                }
                exiteverything();
                menustate++;
                break;


        }
        
        
       
    }
fail:
    consoleInit(GFX_TOP, nullptr);
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            goto deinit;
    }
    
deinit:
    if(soapfinished) {
        if(argc > 1) {
            remove(argv[0]);
        } else {
            amInit();
            AM_DeleteTitle(MEDIATYPE_SD, (u64)0x00040000050AF600);
        }
        ptmSysmInit();
        PTMSYSM_ShutdownAsync(0);
        ptmSysmExit();
    }
    if (sheet)
        C2D_SpriteSheetFree(sheet); 
    exitUI();
    socExit();
    romfsExit();
    gfxExit();
}