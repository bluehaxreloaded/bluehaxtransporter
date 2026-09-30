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

void enter(char* inout, size_t len, SwkbdType kbtype, bool pinInput) {
    SwkbdState swkbd;
    swkbdInit(&swkbd, kbtype, 1, len);
    if(pinInput) {
        swkbdSetValidation(&swkbd, SWKBD_FIXEDLEN, 0, 0);
        swkbdSetHintText(&swkbd, "Pairing PIN");
    }
    swkbdSetFeatures(&swkbd, SWKBD_ALLOW_HOME | SWKBD_ALLOW_POWER);
    swkbdSetInitialText(&swkbd, inout);
    swkbdInputText(&swkbd, inout, len+1);
    return;
}

void showError(std::string message) {
    errorConf err;
    errorInit(&err, ERROR_TEXT_WORD_WRAP, CFG_LANGUAGE_EN);
    errorText(&err, message.c_str());
    errorDisp(&err);
    return;
}

const char* getLoadingFrame(int frame_number) {
    if(frame_number < 3) {
        return "\ue020";
    } else if(frame_number < 6) {
        return "\ue021";
    } else if(frame_number < 9) {
        return "\ue022";
    } else if(frame_number < 12) {
        return "\ue023";
    } else if(frame_number < 15) {
        return "\ue024";
    } else if(frame_number < 18) {
        return "\ue025";
    } else if(frame_number < 21) {
        return "\ue026";
    } else {
        return "\ue027";
    }
}

int main(int argc, char** argv) {
    bool soapfinished = false;
    C2D_SpriteSheet sheet = NULL;
    gfxInitDefault();
    romfsInit();
    

    int result;
    char pairingcode[33] = "";
    char address[52] = "http://soap.gorgerush.net:9023/submit";
    initContext(&ctx);
    initColors(&ctx);
    int menustate = 0;
    std::string finaltext;
    std::string outputData;
    bool usenandessential = false;
    bool setpairingcodeviaserial = false;
    int frame_counter = 0;
    CURLM* multi_handle;
    int request_in_progress = 1;
    int msgs_left = 0;

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
    C2D_Sprite bhlogo, topstart, topbg, bottombg;
    C2D_SpriteFromSheet(&bhlogo, sheet, 0);
    C2D_SpriteFromSheet(&topstart, sheet, 1);
    C2D_SpriteFromSheet(&topbg, sheet, 2);
    C2D_SpriteFromSheet(&bottombg, sheet, 3);

    C2D_SpriteSetPos(&bhlogo, 10, 10);
    C2D_SpriteSetScale(&bhlogo, 0.4, 0.4);
    C2D_SpriteSetCenter(&bhlogo, 0.0, 0.0);

    C2D_SpriteSetPos(&topstart, 0, 0);
    C2D_SpriteSetCenter(&topstart, 0, 0);

    C2D_SpriteSetPos(&topbg, 0, 0);
    C2D_SpriteSetCenter(&topbg, 0, 0);

    C2D_SpriteSetPos(&bottombg, 0, 0);
    C2D_SpriteSetCenter(&bottombg, 0, 0);

    C3D_FrameRate(24);

    

    while (aptMainLoop()) {
        frame_counter++;
        frame_counter = frame_counter % 24;
        hidScanInput();
        touchPosition touch;
        hidTouchRead(&touch);
        u32 kDown = hidKeysHeld();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(ctx.top, ctx.clrBgDark);
        C2D_TargetClear(ctx.bottom, ctx.clrBgDark);
        C2D_SceneBegin(ctx.bottom);
        C2D_DrawSprite(&bottombg);
        C2D_SceneBegin(ctx.top);
        /*
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
        */
        
        switch(menustate) {
            case 0:
                C2D_DrawSprite(&topstart);
                drawText(20, 210, 0, 0.5, ctx.clrWhite, 0, "Server address: %s", address);
                if(setpairingcodeviaserial) {
                    drawText(20, 190, 0, 0.5, ctx.clrWhite, 0, "Username set to match serial");
                }
                C2D_SceneBegin(ctx.bottom);
                drawText(SCREEN_WIDTH_BOTTOM/2, SCREEN_HEIGHT/2+20, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Press \uE000 to begin!");
                drawText(SCREEN_WIDTH_BOTTOM/2, SCREEN_HEIGHT/2+50, 0, 0.4, ctx.clrWhite, C2D_AlignCenter, "Press START to return to the \uE073 HOME Menu.");
                break;
            case 1:
                C2D_DrawSprite(&topbg);
                if(strlen(pairingcode)==0) {
                    drawTextCenter(SCREEN_WIDTH_TOP/2, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Please enter the pairing code\nsent within the Discord server.");
                }
                break;
            case 2:
            case 3:
                C2D_DrawSprite(&topbg);
                drawTextCenter(SCREEN_WIDTH_TOP/2, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Submitting...");
                C2D_SceneBegin(ctx.bottom);
                drawText(SCREEN_WIDTH_BOTTOM/2 - 16, SCREEN_HEIGHT/2 - 16, 0, 1, ctx.clrWhite, 0, getLoadingFrame(frame_counter));
                break;
            case 4:
                
                if (!soapfinished) {
                    //drawText(SCREEN_WIDTH_TOP/2, SCREEN_HEIGHT*3/4, 0, 0.5, ctx.clrRed, C2D_AlignCenter, finaltext.c_str());
                    showError(finaltext.c_str());
                    menustate = 0;
                    pairingcode[0] = '\0';
                    setpairingcodeviaserial = false;
                    continue;
                } else {
                    C2D_DrawSprite(&topbg);
                    drawTextCenter(SCREEN_WIDTH_TOP/2, 0, 0.5, ctx.clrWhite, C2D_AlignCenter, finaltext.c_str());
                    C2D_SceneBegin(ctx.bottom);
                    drawText(SCREEN_WIDTH_BOTTOM/2, SCREEN_HEIGHT/2+20, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Press \uE000 to power off.");
                }
                break;

        }
        C3D_FrameEnd(0);
        switch(menustate) {
            case 4:
                if (kDown & (KEY_START | KEY_A)) {
                    goto deinit;
                }
                break;
            case 0:
                if ((kDown & KEY_X) && (kDown & KEY_DDOWN)) {
                    enter(address, 51, SWKBD_TYPE_NORMAL, false);
                }
                if ((kDown & KEY_L) && (kDown & KEY_DUP)) {
                    sprintf(pairingcode, getNANDEssentialSerial());
                    setpairingcodeviaserial = true;
                }
                 if (strlen(address) > 0 && (kDown & (KEY_TOUCH | KEY_A))) {
                    menustate++;
                }
                if (kDown & KEY_START) {
                    goto deinit;
                }
                break;
            case 1:
                if(strlen(pairingcode) == 0) {
                    enter(pairingcode, 4, SWKBD_TYPE_NUMPAD, true);
                } else {
                    menustate++;
                }
                break;
            case 2:
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
                pairingcodeentry(pairingcode);
                if (usenandessential) {
                    essentialdataentry();
                } else {
                    fileentry("sdmc:/gm9/out/essential.exefs");
                }
                serialentry("sd", getSDEssentialSerial());
                serialentry("nand", getNANDEssentialSerial());
                serialentry("twln", getTWLNSerial());
                serialentry("secinfo", getSecinfoSerial());
                request_in_progress = 1;
                multi_handle = submittourl(address, &outputData);
                menustate++;
                break;
            case 3:
                if(request_in_progress) {
                    curl_multi_perform(multi_handle, &request_in_progress);                    
                }
                if(!request_in_progress) {
                    CURLMsg *msg = curl_multi_info_read(multi_handle, &msgs_left);
                    if(msg && (msg->msg == CURLMSG_DONE)) {
                        if(msg->data.result) {
                            if(msg->data.result == CURLE_COULDNT_RESOLVE_HOST) {
                                finaltext = std::string("cURL error code: 6\n\nSubmission failed: Couldn't resolve host name. Are you connected to the Internet?");
                            } else {
                                finaltext = std::string("cURL error code: ") + std::to_string((int)msg->data.result) + std::string("\n\nSubmission failed: ") + curl_easy_strerror(msg->data.result) + "\n";
                            }
                        } else {
                            long http_code = gethttpcode();
                            if(http_code == 200) {
                                soapfinished = true;
                            }
                            finaltext = outputData;
                        }
                        exiteverything();
                        menustate++;
                    }
                }
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