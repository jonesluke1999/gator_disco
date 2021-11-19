#include <stdio.h>
#include "raylib.h"
#include <math.h>

#define SAMPLE_RATE 44100
#define STREAM_BUFFER_SIZE 1024
#define SCREEN_HEIGHT 768
#define SCREEN_WIDTH 1000
#define WHITE_KEY_WIDTH 100
#define SLIDER_HEIGHT 150
#define SLIDER_WIDTH 30
#define SLIDER_VISIBLE_WIDTH 10

typedef struct{
    float phase;
    float phaseStride;
    float threshold;
    float octave;
    float octaveFactor;
} Oscillator;
typedef struct{
    bool black;
    float frequency;
    bool pressed;
    int xPos;
} Key;
typedef struct{
    float* param;
    float value;
    int xPos;
    int yPos;
    char* name;
} Slider;
typedef struct{
    bool keyPressed;
    int x;
    int y;
} Input;

Key keys[17];
Slider sliders[2];
Oscillator osc;
Input masterInput;
float buffer[1024];
float frequency = 10;
float threshold = 0;
float amplitude = 0;
float sample_duration;

void updateSignal(float* signal, float frequency, float sample_duration, float amplitude){
    if(osc.octave < 0.25){
        osc.octaveFactor = 0.25;
    }
    else if(osc.octave < 0.5){
        osc.octaveFactor = 0.5;
    }
    else if(osc.octave < 0.75){
        osc.octaveFactor = 1;
    }
    else{
        osc.octaveFactor = 2;
    }
    osc.phaseStride = frequency * sample_duration * osc.octaveFactor;
    for(int i = 0; i < 1024; i++){
        osc.phase += osc.phaseStride;
        if(osc.phase > 1) osc.phase -= 1;
        float sin_value = sinf(2.0f * PI * osc.phase);
        if(sin_value > osc.threshold){
            signal[i] = 0.5;
        }
        else {
            signal[i] = -0.5;
        }
        signal[i] *= amplitude;
    }
}
void drawWaveform(float* signal,int width,int height,int x, int y){
    DrawRectangle(x, y, width, height, WHITE);
    int offset = (int)(osc.phase/osc.phaseStride);
    int loop = (int)1.0/osc.phaseStride;
    if (loop > 1024) loop = 1024;
    int start = (STREAM_BUFFER_SIZE-offset)%loop;
    Vector2 prev;
    prev.x = x;
    prev.y = (height/2)+0.5*(int)(signal[0]*100)+y;
    for(int i = 1; i < width - 1; i++){
        int index = (start + (int)(500*i/(frequency*osc.octaveFactor))%loop)%STREAM_BUFFER_SIZE;
        Vector2 current;
        current.x = i+x;
        current.y = (height/2)+0.5*(int)(signal[index]*100)+y;
        DrawLineEx(current, prev, 1.0f, RED);
        prev = current;
    }
}
void buildKeys(){
    int xPos = 0;
    float tempFreq = 261.6;
    for(int i = 0; i < 17; i++){
        Key tempKey;
        tempKey.xPos = xPos;
        tempKey.pressed = false;
        tempKey.frequency = tempFreq;
        int note = i%12;
        if((note < 4 && note%2 == 1)||(note > 5 && note%2 == 0)){
            tempKey.black = true;
            tempKey.xPos = xPos + WHITE_KEY_WIDTH*3/4;
            xPos += WHITE_KEY_WIDTH;
        }
        else{
            tempKey.black = false;
            if(i > 0){
                if(keys[i-1].black == false){
                    xPos += WHITE_KEY_WIDTH;
                }
            }
            tempKey.xPos = xPos;
            
        }
        keys[i] = tempKey;
        tempFreq *= 1.059463;
    }
}
void buildSliders(){
    Slider threshold;
    threshold.xPos = 300;
    threshold.yPos = 100;
    threshold.value = 0;
    threshold.param = &osc.threshold;
    threshold.name = "PULSE WIDTH";
    sliders[0] = threshold;
    Slider octave;
    octave.xPos = 200;
    octave.yPos = 100;
    octave.value = 0;
    octave.param = &osc.octave;
    octave.name = "OCTAVE";
    sliders[1] = octave;
    
}
void drawSliders(){
    for(int i = 0; i < 2; i++){
        Slider tempSlider = sliders[i];
        //draw black rectangle
        int visibleXPos = tempSlider.xPos + SLIDER_WIDTH/2 - SLIDER_VISIBLE_WIDTH/2;
        DrawRectangle(visibleXPos, tempSlider.yPos, SLIDER_VISIBLE_WIDTH, SLIDER_HEIGHT, BLACK);
        //draw white rectangle representing current value
        int whiteRectHeight = SLIDER_HEIGHT*tempSlider.value;
        DrawRectangle(visibleXPos, tempSlider.yPos+SLIDER_HEIGHT-whiteRectHeight, SLIDER_VISIBLE_WIDTH, whiteRectHeight, WHITE);
        //draw circle at end of white rectangle
        //DrawCircle(int centerX, int centerY, float radius, Color color);
        DrawCircle(tempSlider.xPos+SLIDER_WIDTH/2, tempSlider.yPos+SLIDER_HEIGHT-whiteRectHeight, SLIDER_WIDTH/2, RED);
        //draw text
        DrawText(tempSlider.name, tempSlider.xPos, tempSlider.yPos + SLIDER_HEIGHT + 30, 15, BLACK);
    }
}
void drawKeys(int height){
    //draw white keys
    for(int i = 0; i < 17; i++){
        Key tempKey = keys[i];
        if(!tempKey.black){
            if(!tempKey.pressed){
                DrawRectangle(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/10, height, WHITE);
            }
            else{
                DrawRectangle(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/10, height, DARKGRAY);
            }
                DrawRectangleLines(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/10, height, BLACK);
        }
    }
    //draw black keys
    for(int i = 0; i < 17; i++){
        Key tempKey = keys[i];
        if(tempKey.black){
            if(!tempKey.pressed){
                DrawRectangle(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/20, height/2, BLACK);
            }
            else{
                DrawRectangle(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/20, height/2, DARKGRAY);
            }
        }
    }
}
void drawGUI(){
    BeginDrawing();
    ClearBackground(GRAY);
    drawWaveform(buffer,SCREEN_WIDTH/6,SCREEN_HEIGHT/6,SCREEN_WIDTH-(SCREEN_WIDTH*1.5/6),SCREEN_HEIGHT/12);
    drawKeys(SCREEN_HEIGHT/4);
    drawSliders();
    EndDrawing();
}
void clearKeyPress(){
    if(masterInput.keyPressed){
        for(int i = 0; i < 17; i++){
            keys[i].pressed = false;
        }
        masterInput.keyPressed = false;
    }
}
void processInput(){
    masterInput.y = GetMouseY();
    masterInput.x = GetMouseX();
    
    if(IsMouseButtonDown(0)){
        if(masterInput.y > (3*SCREEN_HEIGHT / 4)){
            masterInput.keyPressed = true;
            amplitude = 1;
            bool checkBlack = false;
            bool foundKey = false;
            int keyIndex = -1;
            if(masterInput.y < 7*SCREEN_HEIGHT/8) checkBlack = true;
            for(int i = 0; i < 17; i++){
                keys[i].pressed = false;
                if(!foundKey){
                    Key tempKey = keys[i];
                    if(tempKey.black){
                        if(checkBlack){
                            if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH/2){
                                keyIndex = i;
                                foundKey = true;
                            }
                        }
                    }
                    else{
                        if(!checkBlack){
                            if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH){
                                keyIndex = i;
                                foundKey = true;
                            }
                        }
                        else{
                            if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH*0.75){
                                keyIndex = i;
                                foundKey = true;
                            }
                        }
                    }
                }
            }
            frequency = keys[keyIndex].frequency;
            keys[keyIndex].pressed = true;
        }
        else{
            clearKeyPress();
            //check if slider is selected
            for(int i = 0; i < 2; i++){
                Slider tempSlider = sliders[i];
                if(masterInput.x > tempSlider.xPos && masterInput.x -tempSlider.xPos < SLIDER_WIDTH && masterInput.y > tempSlider.yPos && masterInput.y -tempSlider.yPos < SLIDER_HEIGHT){
                    tempSlider.value = (float)(tempSlider.yPos + SLIDER_HEIGHT - masterInput.y)/SLIDER_HEIGHT;
                    *tempSlider.param = tempSlider.value;
                    sliders[i] = tempSlider;
                }
            }
        }
    }
    else {
        amplitude = 0;
        clearKeyPress();
    }
}
void main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Synth");
    SetTargetFPS(60);
    InitAudioDevice();
    frequency = 200;
    osc.threshold = 0;
    osc.octave = 0;
    buildKeys();
    buildSliders();
    sample_duration = (1.0f)/SAMPLE_RATE;
    osc.phase = 0;
    osc.phaseStride = frequency * sample_duration;
    SetAudioStreamBufferSizeDefault(1024);
    AudioStream synthStream = InitAudioStream(SAMPLE_RATE,
        32 ,
        1
    );
    SetAudioStreamVolume(synthStream, 0.25f);
    PlayAudioStream(synthStream);
    while(WindowShouldClose() == false)
    {
        if(IsAudioStreamProcessed(synthStream)){
            UpdateAudioStream(synthStream, buffer, STREAM_BUFFER_SIZE);
            processInput();
            updateSignal(buffer, frequency, sample_duration, amplitude);
            drawGUI();
        }
    }
    CloseAudioDevice();
    CloseWindow();
}
