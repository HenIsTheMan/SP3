#pragma once

bool endLoop = false;
bool firstCall = 1;
bool leftMB = false;
bool rightMB = false;
float dt = 0.f;
float pitch = 0.f;
float yaw = 0.f;
float lastX = 0.f;
float lastY = 0.f;
float SENS = .05f;
float angularFOV = 45.f;
int optimalWinXPos;
int optimalWinYPos;
int optimalWinWidth;
int optimalWinHeight;
int winWidth;
int winHeight;