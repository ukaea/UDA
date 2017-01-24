//! $LastChangedRevision: 115 $
//! $LastChangedDate: 2009-11-09 09:55:16 +0000 (Mon, 09 Nov 2009) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/include/idamdlm.h $

// Change History
//
// 05Jun2009    DGMuir  Added PGConn extern to allow direct SQL against the IDAM database

#ifndef IdamDlm
#define IdamDlm

// getdat Header
//--------------------------------------------------------------------------

// Useful macro

#define ARRLEN(arr) (sizeof(arr)/sizeof(arr[0]))

// Error management

extern IDL_MSG_BLOCK msg_block;

#define ERROR1  0
#define ERROR2  -1

#define GDE_NO_ARGUMENTS    			1
#define GDE_NO_EXP_NUMBER   			2
#define GDE_NO_SIGNAL_ARGUMENT  		3
#define GDE_BAD_HANDLE      			4
#define GDE_NO_VALID_HANDLE 			5
#define GDE_DATA_HAS_ERROR  			6
#define GDE_UNKNOWN_DATA_TYPE   		7
#define GDE_NO_SUCH_DIMENSION  			8
#define GDE_NO_DATA_TO_RETURN   		9
#define GDE_RANK_TOO_HIGH   			10
#define GDE_HEAP_ALLOC_ERROR    		11
#define GDE_NO_API_IDENTIFIED   		12
#define GDE_NO_DATA_STRUCTURE_TO_RETURN 13
#define GDE_NOT_IMPLEMENTED 			20

#endif
