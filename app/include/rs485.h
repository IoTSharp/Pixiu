#pragma once
int open_dev(char *Dev);
void set_speed(int fd, int speed);
int set_parity(int fd, int databits, int stopbits, int parity);
void config_s(int fd);