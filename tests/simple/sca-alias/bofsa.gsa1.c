/*
 * Copyright (c) 2009 Sun Microsystems, Inc., 4150 Network Circle,     
 * Santa Clara, California 95054, U.S.A.  All rights reserved.  Use is
 * subject to license terms.  This distribution may include materials
 * developed by third parties. Sun, Sun Microsystems and the Sun logo                 
 * are trademarks or registered trademarks of Sun Microsystems, Inc.                 
 * or its subsidiaries, in the U.S. and other countries.      
 */
int main()
{
   int i;
   int a[10];
   int b[9];
   for (i=0; i<10 && a[1]; i++)
       continue;

   if (i>=10) 
       i = i-1;
   /* <bug buffer-overflow> */  b[i] = 0  /* </bug> */;
   return a[i];
}

