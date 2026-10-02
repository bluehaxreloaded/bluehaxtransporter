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
        swkbdSetHintText(&swkbd, "Pair Code");
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
    char paircode[33] = "";
    char address[52] = "http://soap.gorgerush.net:9023/submit";
    initContext(&ctx);
    initColors(&ctx);
    int menustate = 0;
    std::string finaltext;
    std::string outputData;
    bool usenandessential = false;
    bool setpaircodeviaserial = false;
    int frame_counter = 0;
    CURLM* multi_handle;
    int request_in_progress = 1;
    int msgs_left = 0;
    bool touch_let_go = false;
    float slider;

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
    C2D_Sprite bottombg, infoicon;
    C2D_Image topbg, topdots, toplogo;
    C2D_SpriteFromSheet(&bottombg, sheet, 3);
    C2D_SpriteFromSheet(&infoicon, sheet, 4);

    topbg = C2D_SpriteSheetGetImage(sheet, 0);
    topdots = C2D_SpriteSheetGetImage(sheet, 1);
    toplogo = C2D_SpriteSheetGetImage(sheet, 2);

    C2D_SpriteSetPos(&bottombg, 0, 0);
    C2D_SpriteSetCenter(&bottombg, 0, 0);

    C2D_SpriteSetPos(&infoicon, SCREEN_WIDTH_BOTTOM - 62, 0);
    C2D_SpriteSetCenter(&infoicon, 0, 0);

    C3D_FrameRate(24);

    

    while (aptMainLoop()) {
        frame_counter++;
        frame_counter = frame_counter % 24;
        hidScanInput();
        touchPosition touch;
        hidTouchRead(&touch);
        u32 kDown = hidKeysHeld();
        slider = osGet3DSliderState();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(ctx.left, ctx.clrBgDark);
        C2D_TargetClear(ctx.right, ctx.clrBgDark);
        C2D_TargetClear(ctx.bottom, ctx.clrBgDark);
        C2D_SceneBegin(ctx.bottom);
        C2D_DrawSprite(&bottombg);
        
        switch(menustate) {
            case 0:
                C2D_SceneBegin(ctx.left);
                C2D_DrawImageAt(topbg, -4*slider, 0, 0);
                C2D_DrawImageAt(topdots, 0, 0, 0);
                C2D_DrawImageAt(toplogo, 4*slider, 0, 0);
                drawText(20+(4*slider), 190, 0, 0.5, ctx.clrWhite, 0, "Slider: %f", slider);
                drawText(20+(4*slider), 210, 0, 0.5, ctx.clrWhite, 0, "Server address: %s", address);
                if(setpaircodeviaserial) {
                    drawText(20+(4*slider), 190, 0, 0.5, ctx.clrWhite, 0, "Username set to match serial");
                }

                C2D_SceneBegin(ctx.right);
                C2D_DrawImageAt(topbg, 4*slider, 0, 0);
                C2D_DrawImageAt(topdots, 0, 0, 0);
                C2D_DrawImageAt(toplogo, -4*slider, 0, 0);
                drawText(20-(4*slider), 190, 0, 0.5, ctx.clrWhite, 0, "Slider: %f", slider);
                drawText(20-(4*slider), 210, 0, 0.5, ctx.clrWhite, 0, "Server address: %s", address);
                if(setpaircodeviaserial) {
                    drawText(20-(4*slider), 190, 0, 0.5, ctx.clrWhite, 0, "Username set to match serial");
                }

                C2D_SceneBegin(ctx.bottom);
                drawTextCenter(SCREEN_WIDTH_BOTTOM/2, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Press \uE000 to begin!");
                drawText(SCREEN_WIDTH_BOTTOM/2, 210, 0, 0.4, ctx.clrWhite, C2D_AlignCenter, "Press START to return to the \uE073 HOME Menu.");
                C2D_DrawSprite(&infoicon);
                break;
            case 1:
                C2D_SceneBegin(ctx.left);
                C2D_DrawImageAt(topbg, -4*slider, 0, 0);
                C2D_DrawImageAt(topdots, 0, 0, 0);
                if(strlen(paircode)==0) {
                    drawTextCenter(SCREEN_WIDTH_TOP/2 + (4*slider), 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Please enter the Pair Code\nsent within the Discord server.");
                }

                C2D_SceneBegin(ctx.right);
                C2D_DrawImageAt(topbg, 4*slider, 0, 0);
                C2D_DrawImageAt(topdots, 0, 0, 0);
                if(strlen(paircode)==0) {
                    drawTextCenter(SCREEN_WIDTH_TOP/2 - (4*slider), 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Please enter the Pair Code\nsent within the Discord server.");
                }
                break;
            case 2:
            case 3:
                C2D_SceneBegin(ctx.left);
                C2D_DrawImageAt(topbg, -4*slider, 0, 0);
                C2D_DrawImageAt(topdots, 0, 0, 0);
                drawTextCenter(SCREEN_WIDTH_TOP/2 + (4*slider), 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Submitting...");

                C2D_SceneBegin(ctx.right);
                C2D_DrawImageAt(topbg, 4*slider, 0, 0);
                C2D_DrawImageAt(topdots, 0, 0, 0);
                drawTextCenter(SCREEN_WIDTH_TOP/2 - (4*slider), 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Submitting...");

                C2D_SceneBegin(ctx.bottom);
                drawTextCenter(SCREEN_WIDTH_BOTTOM/2, 0, 1, ctx.clrWhite, C2D_AlignCenter, getLoadingFrame(frame_counter));
                break;
            case 4:
                
                if (!soapfinished) {
                    //drawText(SCREEN_WIDTH_TOP/2, SCREEN_HEIGHT*3/4, 0, 0.5, ctx.clrRed, C2D_AlignCenter, finaltext.c_str());
                    showError(finaltext.c_str());
                    menustate = 0;
                    paircode[0] = '\0';
                    setpaircodeviaserial = false;
                    continue;
                } else {
                    C2D_SceneBegin(ctx.left);
                    C2D_DrawImageAt(topbg, -4*slider, 0, 0);
                    C2D_DrawImageAt(topdots, 0, 0, 0);
                    drawTextCenter(SCREEN_WIDTH_TOP/2 + (4*slider), 0, 0.5, ctx.clrWhite, C2D_AlignCenter, finaltext.c_str());

                    C2D_SceneBegin(ctx.right);
                    C2D_DrawImageAt(topbg, 4*slider, 0, 0);
                    C2D_DrawImageAt(topdots, 0, 0, 0);
                    drawTextCenter(SCREEN_WIDTH_TOP/2 - (4*slider), 0, 0.5, ctx.clrWhite, C2D_AlignCenter, finaltext.c_str());

                    C2D_SceneBegin(ctx.bottom);
                    drawTextCenter(SCREEN_WIDTH_BOTTOM/2, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Press \uE000 to power off.");
                }
                break;
            case 5:
                C2D_SceneBegin(ctx.left);
                C2D_DrawImageAt(topbg, -4*slider, 0, 0);
                C2D_DrawImageAt(topdots, 0, 0, 0);
                C2D_DrawImageAt(toplogo, 4*slider, 0, 0);
                drawText(SCREEN_WIDTH_TOP/2 + (4*slider), 200, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Application Info");
                
                C2D_SceneBegin(ctx.right);
                C2D_DrawImageAt(topbg, 4*slider, 0, 0);
                C2D_DrawImageAt(topdots, 0, 0, 0);
                C2D_DrawImageAt(toplogo, -4*slider, 0, 0);
                drawText(SCREEN_WIDTH_TOP/2 - (4*slider), 200, 0, 0.7, ctx.clrWhite, C2D_AlignCenter, "Application Info");

                C2D_SceneBegin(ctx.bottom);
                drawTextCenter(SCREEN_WIDTH_BOTTOM/2, 0, 0.4, ctx.clrWhite, C2D_AlignCenter,
                    "Created by the Bluehax team\n"
                    "Original essentialsubmit application created by gruetzig\n"
                    "SD essential.exefs serial: %s\n"
                    "NAND essential.exefs serial: %s\n"
                    "SecureInfo serial: %s\n"
                    "inspect.log serial: %s\n",
                    getSDEssentialSerial(),
                    getNANDEssentialSerial(),
                    getSecinfoSerial(),
                    getTWLNSerial()
                );\
                drawText(SCREEN_WIDTH_BOTTOM/2, 210, 0, 0.4, ctx.clrWhite, C2D_AlignCenter, "Press \uE001 to return to the main menu.");
                break;

        }
        C3D_FrameEnd(0);
        if(!(kDown & KEY_TOUCH) && !touch_let_go) {
            touch_let_go = true;
        }
        switch(menustate) {
            case 4:
                if (kDown & (KEY_START | KEY_A)) {
                    goto deinit;
                }
                break;
            case 5:
                if (kDown & KEY_START) {
                    goto deinit;
                }
                if (touch_let_go && (kDown & (KEY_TOUCH | KEY_B))) {
                    menustate = 0;
                    touch_let_go = false;
                }
                break;
            case 0:
                if ((kDown & KEY_X) && (kDown & KEY_DDOWN)) {
                    enter(address, 51, SWKBD_TYPE_NORMAL, false);
                }
                if ((kDown & KEY_L) && (kDown & KEY_DUP)) {
                    sprintf(paircode, getNANDEssentialSerial());
                    setpaircodeviaserial = true;
                }
                if (strlen(address) > 0 && (kDown & KEY_A)) {
                    menustate++;
                }
                if(touch_let_go && (kDown & KEY_TOUCH)) {
                    if(touch.px >= (SCREEN_WIDTH_BOTTOM - 70) && touch.py <= 50) {
                        touch_let_go = false;
                        menustate = 5;
                    } else if(strlen(address) > 0) {
                        menustate++;
                    }
                }
                if (kDown & KEY_START) {
                    goto deinit;
                }
                break;
            case 1:
                if(strlen(paircode) == 0) {
                    enter(paircode, 4, SWKBD_TYPE_NUMPAD, true);
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
                paircodeentry(paircode);
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