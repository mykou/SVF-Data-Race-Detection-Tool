//============================================================================
// Name        : unInstrumented.cpp
// Author      : Vishv Malhotra
// Version     :
// Copyright   : Your copyright notice
// Description : Sort program to test performance
// degradation from long pointer idea against SoftBound
//============================================================================

#include <stdio.h>
#include <stdlib.h>

void mySort(int a[], int SZ) {
    int i, j;

    for (i=0; i<SZ-1; i++) {
        for (j=i+1; j<SZ; j++) {
            if (a[i]<a[j]) {
                int temp = a[i];
                a[i] = a[j];
                a[j]= temp;
             }
        }
    }
}

int main(void) {

    #define MAX 100000

    int *globalData = malloc(sizeof(int) * MAX);
    if (globalData) {    

        // initialize the array -- not timed
        int  i;
        for (i=0; i<MAX; i++) {
            globalData[i] = i%7654321+i%654321;
        }
        mySort(globalData, MAX);
        for (i=0; i<MAX; i = i +MAX/10) {
            printf("%i\n", globalData[i]);
        }

        free(globalData);
    }

    return 0;
}

