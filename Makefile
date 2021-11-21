c: src/alarm.c src/alarm.h src/app.c src/app.h src/constants.h src/data_protocol.c src/data_protocol.h src/protocol_app.c src/protocol_app.h src/receiver.c src/stuffing.c src/stuffing.h src/transmitter.c
	gcc -o transmitter src/alarm.c src/app.c src/data_protocol.c src/protocol_app.c src/stuffing.c src/transmitter.c
	gcc -o receiver src/alarm.c src/app.c src/data_protocol.c src/protocol_app.c src/stuffing.c src/receiver.c


clean:
	rm -f c

