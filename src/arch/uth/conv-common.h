
#define CMK_MSG_HEADER_FIELD  CmiUInt2 hdl,xhdl,info,stratid,root,pad1,pad2,pad3;
#define CMK_MSG_HEADER_BASIC  CMK_MSG_HEADER_EXT
#define CMK_MSG_HEADER_EXT    { CMK_MSG_HEADER_FIELD }
#define CMK_MSG_HEADER_BLUEGENE    { CMK_MSG_HEADER_FIELD CMK_BLUEGENE_FIELDS }

#define CMK_LBDB_ON					   0
