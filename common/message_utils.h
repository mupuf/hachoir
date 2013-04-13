#ifndef MESSAGEUTILS_H
#define MESSAGEUTILS_H

/* from http://stackoverflow.com/questions/10394951/parsing-data-into-struct */
template<typename T>
void read_and_update_offset (size_t & offset, const char * buffer, T & var)
{
	T * pInt = (T*)(buffer + offset);
	var = *pInt;
	offset += sizeof(T);
}

template<typename T>
void write_and_update_offset (size_t & offset, char * buffer, const T & var)
{
	T * pInt = (T*)(buffer + offset);
	*pInt = var;
	offset += sizeof(T);
}

enum MessageType { MSG_FFT = 1, MSG_RET = 2 };

#endif
