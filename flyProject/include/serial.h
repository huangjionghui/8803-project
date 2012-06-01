#ifndef SERIAL_H_
#define SERIAL_H_



extern int serial_open(void);
extern int serial_close(int fd);
extern long serial_read(int fd, unsigned char *buf, int len);
extern int serial_write(int fd, unsigned char *buf, int len);



#endif