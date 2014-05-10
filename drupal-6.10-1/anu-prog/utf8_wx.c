//**************************************
// FILE     : utf8_wx.c
// Contact  : rohit@research.iiit.ac.in
// PART OF  : Convertor utf to wx 
// CREATED  : 15th June, 2009
//
// MODIFIED LAST : 15th June, 2009
//***************************************

/*
 * This converter converts hindi text from utf to wx. The main function calls the function utf_wx for the conversion of each line. The line is then read
 * character by character and the conversion is done according to the language rules regarding consonants, matra and vowels using the caracter boundaries
 * defined next and the map defined in form of an array after the character boundaries.
 */

#include <stdio.h>				// Include file for Library for standard i/o
#include <stdlib.h>				// Include file for Library for std library functions	
#include <string.h>				// Include file for String

/*
 * Define charactor boundries
 */
#define HIN_START 128
#define MATRA_START1 1
#define MATRA_END1 3 
#define MATRA_START2 60
#define MATRA_END2 60
#define MATRA_START3 62
#define MATRA_END3 76

#define VOW_FIRST_START1 5
#define VOW_FIRST_END1 11
#define VOW_FIRST_START2 15
#define VOW_FIRST_END2 16
#define VOW_FIRST_START3 19
#define VOW_FIRST_END3 20

#define CON_START 21
#define CON_END 57

#define NUM_START 96
#define NUM_END 111


void utf_wx(unsigned char *ch);				// function to convert from utf to wx

char eng[] = {' ', 'z', 'M', 'H', ' ', 'a','A','i','I','u','U', 'q', 'L', ' ', ' ', 'e', 'E', ' ', ' ', 'o', 'O', 'k', 'K', 'g', 'G', 'f', 'c', 'C', 'j', 'J', 'F', 't', 'T', 'd', 'D', 'N', 'w', 'W', 'x', 'X', 'n', 'n', 'p', 'P', 'b', 'B', 'm', 'y', 'r', 'r', 'l', ' ', ' ', 'v', 'S', 'R', 's', 'h', ' ', ' ', 'Z', ' ', 'A', 'i', 'I', 'u', 'U', 'q', 'Q', ' ', ' ', 'e', 'E', ' ', ' ', 'o', 'O', ' ', ' ', '`', '\'', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 'Q', ' ', ' ', ' ', '.', '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

int main()
{
	int bytes_read;			
//	int nbytes = 10000;
        size_t nbytes = 10000;
	char *my_string;
	while(1){
  		my_string = (char *) malloc (nbytes + 1);
		if (my_string == NULL) {
			printf("Can't allocate memory\n");
			break;
		}

		bytes_read = getline (&my_string, &nbytes, stdin);

		if (bytes_read == -1) {
			break;
		} else {
			utf_wx(my_string);
		 }
		 free(my_string);
	}

	return 0;
}

void utf_wx(unsigned char *ch) {
	unsigned char ch1[4];
	unsigned char ch2[4];
	unsigned char ch3[4];
	int i = 0;
	int k;
	int a;
	int b;
	int index;
	
		
	while ( ch1[0] = *(ch+i) ) {
		if (ch1[0] == '\n') {
			printf("\n");
			return;
		} else if (ch1[0] == '\0') {
			i++;
			continue;
 		} else if (isascii(ch1[0])) {
			printf("%c", ch1[0]);
			i++;
			continue;
		}
		ch1[1] = *(ch+i+1);
		ch1[2] = *(ch+i+2);
		ch1[3] = '\0';
		i += 3;
		
		if (ch1[1] == 164) {
			index = ch1[2] - HIN_START;
			if ((index >= VOW_FIRST_START1 && index <= VOW_FIRST_END1) || (index >= VOW_FIRST_START2 && index <= VOW_FIRST_END2) || (index >= VOW_FIRST_START3 && index <= VOW_FIRST_END3)) {
				printf("%c", eng[index]);
			} else if ((index >= MATRA_START1 && index <= MATRA_END1) || (index >= MATRA_START2 && index <= MATRA_END2) || (index >= MATRA_START3 && index <= MATRA_END3)) {
					printf("%c", eng[index] );
					continue;
			} else {
				printf("%c", eng[index]);
				ch2[0] = *(ch+i);
				if (ch2[0] == '\n') {
					printf("a\n");
					return;
				} else if (ch2[0] == '\0') {
					i++;
					continue;
		 		} else if (isascii(ch2[0])) {
					printf("a%c", ch2[0]);
					i++;
					continue;
				}

				ch2[1] = *(ch+i+1);
				ch2[2] = *(ch+i+2);
				ch2[3] = '\0';

				if (ch2[1] == 164) {
					index = ch2[2] - HIN_START;
					if ((index >= MATRA_START3 && index <= MATRA_END3)) {
						printf("%c", eng[index] );
						i += 3;
					} else if (index == MATRA_START2) {
						printf("%c", eng[index]);
						i += 3;
						ch3[0] = *(ch+i);
						if (ch3[0] == '\n') {
							printf("a\n");
						} else if (ch3[0] == '\0') {
							i++;
							continue;
				 		} else if (isascii(ch3[0])) {
							printf("a%c", ch3[0]);
							i++;
							continue;
						}

						ch3[1] = *(ch+i+1);
						ch3[2] = *(ch+i+2);
						ch3[3] = '\0';
			
						if (ch3[1] == 164) {
							index = ch3[2] - HIN_START;
							if ((index != 2) && (index >= MATRA_START1 && index <= MATRA_END1) || (index >= MATRA_START3 && index <= MATRA_END3)) {
								printf("%c", eng[index] );
								i += 3;
								continue;
							} else if (index == 2) {
								printf("a");
							} else if (index >= CON_START && index <= CON_END) {
								printf("a");
								continue;
							} else {								
								continue;
							}
						} else if (ch3[1] == 165) {
							index = ch3[2] - HIN_START + 64;
							if ((index >= MATRA_START3 && index <= MATRA_END3)) {
								printf("%c", eng[index] );
								i += 3;
							} else {
								i += 3;
								continue;
							}
						} else {
							printf ("%s", ch3);
						} 
					} else {
						printf("a");
					}
				} else if (ch2[1] == 165) {
					index = 64 + ch2[2] - HIN_START;
					if ((index >= MATRA_START2 && index <= MATRA_END2) || (index >= MATRA_START3 && index <= MATRA_END3)) {
						printf("%c", eng[index] );
						i += 3;
					} else if (index == (141 + 64 - HIN_START)) {
						i +=3;
						continue;
					} else {
						printf("a"); 			 
					}
				}
			}
		} else if (ch1[1] == 165) {
			index = 64 + ch1[2] - HIN_START;
			if (index >= NUM_START && index <= NUM_END) {
				printf("%c", eng[index]);
			}
		} else {

			printf("%s", ch1);
		}

	}
}
