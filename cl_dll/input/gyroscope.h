#ifndef GYROSCOPE_H
#define GYROSCOPE_H

void Gyro_Init(void);
void Gyro_Shutdown(void);
void Gyro_Update(float *yaw, float *pitch);
void Gyro_Reset(void);
int Gyro_IsEnabled(void);

#endif
