#ifndef APP_H
#define APP_H

struct applicationLayer
{
    int sequenceNumber;
};

struct applicationLayer app_info;

/**
    * Function that creates application data package.
     * @param n: sequence number.
     * @param length: size of data array.
     * @param data: contains bytes to be sent.
     * @param package: array to store built package.
     * @return: 0 or -1 in case of error.
*/
int create_data_package(int n, int length, u_int8_t *data, u_int8_t *package);

/**
    * Function that reads application data package.
     * @param data: array to store data contained in data package.
     * @param package: data package that function is going to read and extract information from.
     * @return: size of data read or -1 in case of error.
*/
int read_data_package(u_int8_t*data, u_int8_t *package);

/**
    * Function that creates application control package.
     * @param c: control byte (can be start or end).
     * @param file_name: name of the file that will be sent.
     * @param length: size of the file that will be sent.
     * @param package: array to store built package.
     * @return: size of the control package or -1 in case of error.
*/
int create_control_package(u_int8_t c, u_int8_t *file_name, int length, u_int8_t*package);

/**
    * Function that reads application control package.
     * @param package: control package to be read.
     * @param file_name: variable used to store the name of the file (information in control package).
     * @param file_size: variable used to store size of the file (information in control package).
     * @param package_size: size of the control package.
     * @return: 0 or -1 in case of error.
*/
int read_control_package(u_int8_t *package, u_int8_t *file_name, int *file_size, int package_size);

#endif
