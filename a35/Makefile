CC      = $(CROSS_COMPILE)gcc
CFLAGS  = -O2 -Wall -Wextra -std=gnu99 -I..

rpmsg_sensor_rx: rpmsg_sensor_rx.c ../protocol.h
	$(CC) $(CFLAGS) -o $@ $<

selftest: rpmsg_sensor_rx.c ../protocol.h
	$(CC) $(CFLAGS) -DSELFTEST -o $@ $<

test: selftest
	./selftest

clean:
	rm -f rpmsg_sensor_rx selftest

.PHONY: test clean
