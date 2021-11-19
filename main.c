#include <stdio.h>
#include "raylib.h"
#include <math.h>

#define SAMPLE_RATE 44100
#define STREAM_BUFFER_SIZE 1024
#define SCREEN_HEIGHT 768
#define SCREEN_WIDTH 1000
#define WHITE_KEY_WIDTH 100

typedef struct{
    float phase;
    float phaseStride;
} Oscillator;
typedef struct{
    bool black;
    float frequency;
    bool pressed;
    int xPos;
} Key;
typedef struct{
    bool pressed;
    int x;
    int y;
} Input;

Key keys[17];
Oscillator osc;
Input masterInput;
float buffer[1024];
float frequency = 10;
float threshold = 0;
float amplitude = 0;
float sample_duration;

void updateSignal(float* signal, float frequency, float threshold, float sample_duration, Oscillator* osc, float amplitude){
    osc->phaseStride = frequency * sample_duration;
    for(int i = 0; i < 1024; i++){
        osc->phase += osc->phaseStride;
        if(osc->phase > 1) osc->phase -= 1;
        float sin_value = sinf(2.0f * PI * osc->phase);
        if(sin_value > threshold){
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
    for(int i = 0; i < width - 1; i++){
        int index = (start + i%loop)%STREAM_BUFFER_SIZE;
        Vector2 v1;
        Vector2 v2;
        v1.x = i+x;
        v1.y = (height/2)+0.5*(int)(signal[index]*100)+y;
        v2.x = i+1+x;
        v2.y = (height/2)+0.5*(int)(signal[(index+1)%STREAM_BUFFER_SIZE]*100)+y;
        DrawLineEx(v1, v2, 1.0f, RED);
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
void drawKeys(int height){
    //draw white keys
    for(int i = 0; i < 17; i++){
        Key tempKey = keys[i];
        if(!tempKey.black){
            DrawRectangle(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/10, height, WHITE);
            DrawRectangleLines(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/10, height, BLACK);
        }
    }
    //draw black keys
    for(int i = 0; i < 17; i++){
        Key tempKey = keys[i];
        if(tempKey.black){
            DrawRectangle(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/20, height/2, BLACK);
        }
    }
}
void drawGUI(){
    BeginDrawing();
    ClearBackground(GRAY);
    drawWaveform(buffer,SCREEN_WIDTH/6,SCREEN_HEIGHT/6,SCREEN_WIDTH-(SCREEN_WIDTH*1.5/6),SCREEN_HEIGHT/12);
    drawKeys(SCREEN_HEIGHT/4);
    EndDrawing();
}
void main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Synth");
    SetTargetFPS(60);
    InitAudioDevice();
    frequency = 200;
    threshold = 0;
    buildKeys();
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
            masterInput.y = GetMouseY();
            masterInput.x = GetMouseX();
            if(masterInput.y > (3*SCREEN_HEIGHT / 4) && IsMouseButtonDown(0)){
                amplitude = 1;
                bool checkBlack = false;
                bool foundKey = false;
                if(masterInput.y < 7*SCREEN_HEIGHT/8) checkBlack = true;
                for(int i = 0; i < 17; i++){
                    if(!foundKey){
                        Key tempKey = keys[i];
                        if(tempKey.black){
                            if(checkBlack){
                                if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH/2){
                                    frequency = tempKey.frequency;
                                    foundKey = true;
                                }
                            }
                        }
                        else{
                            if(!checkBlack){
                                if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH){
                                    frequency = tempKey.frequency;
                                    foundKey = true;
                                }
                            }
                            else{
                                if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH*0.75){
                                    frequency = tempKey.frequency;
                                    foundKey = true;
                                }
                            }
                        }
                    }
                }
                
            }
            else{
                amplitude = 0;
            }
            
            threshold = 0;
            updateSignal(buffer, frequency, threshold, sample_duration, &osc, amplitude);
            UpdateAudioStream(synthStream, buffer, STREAM_BUFFER_SIZE);
            drawGUI();
        }
    }
    CloseAudioDevice();
    CloseWindow();
}
