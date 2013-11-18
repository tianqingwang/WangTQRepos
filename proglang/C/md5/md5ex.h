/*****************************************************************************
* Copyright (c) 2001 [Organization Name],Inc. All rights reserved.
******************************************************************************
 
    $md5.h$
 
    Version Number - $Revision: 1.2 $
    Last Updated   - $Date: 2010/10/29 03:32:08 $
    Updated By     - $Author: liudp $
 
    Message-Digest Algorithm 5
 
    The algorithm takes as input a message of arbitrary length and produces as 
    output a 128-bit "fingerprint" or "message digest" of the input.
    
******************************************************************************
* This file contains trade secrets of [Organization Name]. No part
* may be reproduced or transmitted in any form by any means or for any purpose
* without the express written permission of [Organization Name].
*****************************************************************************/
#ifndef _md5_H_
#define _md5_H_

/*****************************************************************************
    Include Files
*/
//#include<string.h>


/******************************************************************************

   This package supports both compile-time and run-time determination of CPU
   byte order.  If ARCH_IS_BIG_ENDIAN is defined as 0, the code will be
   compiled to run only on little-endian CPUs; if ARCH_IS_BIG_ENDIAN is
   defined as non-zero, the code will be compiled to run only on big-endian
   CPUs; if ARCH_IS_BIG_ENDIAN is not defined, the code will be compiled to
   run on either big- or little-endian CPUs, but will run slightly less
   efficiently on either one than if ARCH_IS_BIG_ENDIAN is defined.
*/


/*****************************************************************************
    Data type Declarations
*/
typedef unsigned char md5_byte_t; /* 8 -bit byte */
typedef unsigned int  md5_word_t; /* 32-bit word */

/*-------------------------------------------------
 Define the state of the MD5 Algorithm. 
 -------------------------------------------------*/
typedef struct md5_state_s {
    md5_word_t count[2];	/* message length in bits, lsw first */
    md5_word_t abcd[4];		/* digest buffer */
    md5_byte_t buf[64];		/* accumulate block */
} md5_state_t;


/*-------------------------------------------------
 Define the output vector of length 128
 -------------------------------------------------*/
struct md5_long_128{
 
	 union{
	    md5_byte_t    byteData[16];  /*data stored in md5-byte format*/
	    unsigned int  intData [4];   /*data stored in int format*/
	 }data;                       /*output vector union of Md5 Algorithm*/

         /*-------------------------------------
           compare fuction definition of
           output vector of length 128 
         --------------------------------------*/
     	 bool 
     	 operator ==(
     	     const md5_long_128 & a   /*the other md5 output vector of length 128*/
     	 )const 		      
     	 {		
		return 	(data.intData[0]==a.data.intData[0])&&
		 	(data.intData[1]==a.data.intData[1])&&
			(data.intData[2]==a.data.intData[2])&&
			(data.intData[3]==a.data.intData[3]);
		 }

};


/*-------------------------------------------------
 Define the output vector of length 64
 -------------------------------------------------*/
struct md5_long_64{
	
	 union{
	     md5_byte_t    byteData[8];    /*data stored in md5-byte format*/
	     unsigned int  intData [2];     /*data stored in int format*/ 
	 }data;			         /*output vector union of Md5 Algorithm*/


         /*-------------------------------------
	   compare fuction definition of
	   output vector of length 64 
	 --------------------------------------*/
         bool 
         operator ==(
             const md5_long_64 & a      /*the other md5 output vector of length 64*/
         )const 
         {		
		return 	(data.intData[0]==a.data.intData[0])&&
			(data.intData[1]==a.data.intData[1]);
  	 }

	/*
	 *	关系运算
	 */
	bool operator < (const md5_long_64& right) const
	{
		if(data.intData[0] < right.data.intData[0])
			return true;
		if(data.intData[0] > right.data.intData[0])
			return false;
		if(data.intData[1] < right.data.intData[1])
			return true;
		return false;
	}
	
	bool operator <= (const md5_long_64& right) const
	{
		if(data.intData[0] < right.data.intData[0])
			return true;
		if(data.intData[0] > right.data.intData[0])
			return false;
		if(data.intData[1] <= right.data.intData[1])
			return true;
		return false;
	}

	bool operator > (const md5_long_64& right) const
	{
		if(data.intData[0] > right.data.intData[0])
			return true;
		if(data.intData[0] < right.data.intData[0])
			return false;
		if(data.intData[1] > right.data.intData[1])
			return true;
		return false;
	}

	bool operator >= (const md5_long_64& right) const
	{
		if(data.intData[0] > right.data.intData[0])
			return true;
		if(data.intData[0] < right.data.intData[0])
			return false;
		if(data.intData[1] >= right.data.intData[1])
			return true;
		return false;
	}

	bool operator != (const md5_long_64& right) const
	{
		if(data.intData[0] != right.data.intData[0])
			return true;
		if(data.intData[1] != right.data.intData[1])
			return true;
		return false;
	}
};


/*-------------------------------------------------
 Define the output vector of length 32
 -------------------------------------------------*/
struct md5_long_32{
	 
	union{
	     md5_byte_t     byteData[4];	/*data stored in md5-byte format*/
	     unsigned  int  intData[1];	/*data stored in int format*/ 
	}data;				/*output vector union of Md5 Algorithm*/

	/*-------------------------------------
	   compare fuction definition of
	   output vector of length 32 
	 --------------------------------------*/
     	bool 
     	operator ==(
     	     const md5_long_32 & a	/*the other md5 output vector of length 32*/
     	)const 
     	{		
		return (data.intData[0]==a.data.intData[0]);
	}

		/*
		 *	关系运算
		 */
		bool operator < (const md5_long_32& right) const
		{
			if(data.intData[0] < right.data.intData[0])
				return true;
			return false;
		}

};


/*****************************************************************************
    Function Declarations
*/

#ifdef __cplusplus
extern "C" 
{
#endif

/* --------------------------------------------------------------------------
   Initialize the algorithm. 
 */
//void md5_init(md5_state_t *pms);

/* --------------------------------------------------------------------------
   Append a string to the message. 
 */
//void md5_append(md5_state_t *pms, const md5_byte_t *data, int nbytes);


/* --------------------------------------------------------------------------
   Finish the message and return the digest. 
 */
//void md5_finish(md5_state_t *pms, md5_byte_t digest[16]);


/* --------------------------------------------------------------------------
   Finish the message and return the digest. 
 */
void md5_finish_1(md5_state_t *pms, md5_byte_t digest[], int size);


/* --------------------------------------------------------------------------
 * Get the output vector of length 128 with the input str length. 
 *
 *   @param 	inputStr    [in]	the string input to calculate its signature
 *   @param 	inputLen    [in]	if inputLen<0 then the inputLen=strlen(inputStr)
 *				        else  use itself which is set by the user.
 *				        [ default is -1 ]
 *
 *   return 	digest      the 128-bits signature of the inputStr with the length inpuLen or strlen(inputStr)
 *
 */
md5_long_128 getSign128(const char * inputStr,  int inputLen=-1);


/* --------------------------------------------------------------------------
 * Get the output vector of length 64 with the input str length. 
 *
 *   @param 	inputStr    [in]	the string input to calculate its signature
 *   @param 	inputLen    [in]	if inputLen<0 then the inputLen=strlen(inputStr)
 *				        else  use itself which is set by the user.
 *				        [ default is -1 ]
 *
 *   return 	digest      the 64-bits signature of the inputStr with the length inpuLen or strlen(inputStr)
 *
 */
md5_long_64  getSign64 (const char * inputStr,  int inputLen=-1);


/* --------------------------------------------------------------------------
 * Get the output vector of length 32 with the input str length. 
 *
 *   @param 	inputStr    [in]	the string input to calculate its signature
 *   @param 	inputLen    [in]	if inputLen<0 then the inputLen=strlen(inputStr)
 *				        else  use itself which is set by the user.
 *				        [ default is -1 ]
 *
 *   return 	digest      the 32-bits signature of the inputStr with the length inpuLen or strlen(inputStr)
 *
 */
md5_long_32  getSign32 (const char * inputStr,  int inputLen=-1);


#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif /* _md5_H */
