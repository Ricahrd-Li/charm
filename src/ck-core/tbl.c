/***************************************************************************
 * RCS INFORMATION:
 *
 *	$RCSfile$
 *	$Author$	$Locker$		$State$
 *	$Revision$	$Date$
 *
 ***************************************************************************
 * DESCRIPTION:
 *
 ***************************************************************************
 * REVISION HISTORY:
 *
 * $Log$
 * Revision 2.5  1995-07-27 20:29:34  jyelon
 * Improvements to runtime system, general cleanup.
 *
 * Revision 2.4  1995/07/24  01:54:40  jyelon
 * *** empty log message ***
 *
 * Revision 2.3  1995/07/22  23:45:15  jyelon
 * *** empty log message ***
 *
 * Revision 2.2  1995/06/29  22:00:56  narain
 * Changed name of member in PSEDO_STRUCT to tbl
 *
 * Revision 2.1  1995/06/08  17:07:12  gursoy
 * Cpv macro changes done
 *
 * Revision 1.3  1995/04/13  20:55:22  sanjeev
 * Changed Mc to Cmi
 *
 * Revision 1.2  1994/12/01  23:58:04  sanjeev
 * interop stuff
 *
 * Revision 1.1  1994/11/03  17:38:57  brunner
 * Initial revision
 *
 ***************************************************************************/
static char ident[] = "@(#)$Header$";
#include "chare.h"
#include "table.h"
#include "globals.h"
#include "performance.h"

#define SIZE_CHARE_ID sizeof(ChareIDType)


typedef struct tbl_element *TBL_ELEMENT_[MAX_TBL_SIZE];
CpvStaticDeclare(TBL_ELEMENT_, table);


void tblModuleInit()
{
    CpvInitialize(TBL_ELEMENT_, table);
}





TblBocInit()
{
    	BOC_BLOCK *bocBlock;
	int i;

    	bocBlock = (BOC_BLOCK *) CreateBocBlock(sizeof(DATA_BR_TBL));
    	/*TRACE(CmiPrintf("Node %d: TblBocInit:created block with size %d\n",CmiMyPe(), sizeof(DATA_BR_VID))); */
	bocBlock->boc_num = TblBocNum;
    	SetBocDataPtr(TblBocNum, (void *) (bocBlock + 1));

	for (i=0; i<MAX_TBL_SIZE; i++)
		CpvAccess(table)[i] = (TBL_ELEMENT *) NULL;
    	TRACE(CmiPrintf("Node %d: TblBocInit: BocDataTbl entry filled.\n",CmiMyPe()));
}


/************************************************************************/
/*	This entry point unpacks the packed stuff and does the required */
/*	operation.							*/
/************************************************************************/

void Unpack(ptr, dataptr)
char *ptr;
void *dataptr;
{
	int size;
	int *operation, *tbl, *entry, *option;
	int *index;
	char  *data;
	int *key;
	int  *size_data;
	ChareIDType *chareid;
	TBL_MSG *msg;
	int *size_chareid;


TRACE(CmiPrintf("[%d] Unpack:: ptr=0x%x\n", CmiMyPe(), ptr));

	size = sizeof(int);
	operation  = (int *) ptr;
	ptr += size;
	tbl  = (int *)ptr;
	ptr += size;
	index = (int *)ptr;
	ptr += size;
	key = (int *) ptr;
	ptr += size;
	size_data = (int *) ptr;
	ptr += size;
	if (*size_data == 0)
		data = (char *) NULL;
	else
	{
		data = (char *)  ptr;
		ptr += *size_data;
	}
	entry  =(int *) ptr;
	ptr += size;

TRACE(CmiPrintf("[%d] Unpack:: index=%d, key=%d, size_data=%d, entry=%d\n",
		CmiMyPe(), *index, *key, *size_data, *entry));

	size_chareid = (int *) ptr;
	ptr += size;

TRACE(CmiPrintf("[%d] Unpack:: size_chareid=%d\n",
		CmiMyPe(), *size_chareid));

	if (*size_chareid == 0)
		chareid = (ChareIDType *) NULL;
	else
	{
		chareid = (ChareIDType *) ptr; 	
		ptr += *size_chareid;
	}
	option = (int *) ptr;

TRACE(CmiPrintf("[%d] Unpack:: option=%d\n",
		CmiMyPe(), *option));

	switch (*operation){
	case 0 :
		TblInsert(*tbl, *index, *key, data, *size_data,
			 *entry, chareid, *option);
		break;
	case 1:
		TblDelete(*tbl, *index, *key, *entry, chareid, *option);
		break;
	case 2:
		TblFind(*tbl, *index, *key,  *entry, chareid, *option);
		break;
	case 3:
		msg = (TBL_MSG *) CkAllocMsg(sizeof(TBL_MSG));
		CkMemError(msg);
		msg->key = *((int *) key);
		msg->data = data;
		SendMsg(*entry, msg, chareid);
		break;
	default:
		CmiPrintf("We are in trouble here\n");
	}
}


absolute(i)
int i;
{
	if (i<0)
		return(-i);
	else
		return(i);
}


/************************************************************************/
/*	This function copies structures.				*/
/************************************************************************/

structure_copy(x, y, size)
char *x, *y;
int size;
{
	int i;

	for (i=0; i<size; i++)
		*x++ = *y++;
}


/************************************************************************/
/*	This function takes a key and its size, and computes a mapping	*/
/*	(p,i), where p is the processor on which the key resides/will 	*/
/*	reside, and i is the index in the table.			*/
/************************************************************************/

map * Hash(tbl, key)
int tbl, key;
{
	int p, i;
	map *value;
	
	if (CsvAccess(PseudoTable)[tbl].pseudo_type.tbl.hashfn)
		p = (*CsvAccess(PseudoTable)[tbl].pseudo_type.tbl.hashfn)(key);
	else
		p = 13*key % MAX_TBL_SIZE; 
		
	i = 83*key;
	value = (map *) CmiAlloc(sizeof(map));
	CkMemError(value);
	value->penum = absolute(p % CmiNumPe());
	value->index = absolute(i % MAX_TBL_SIZE);
	return(value);
}


/************************************************************************/
/*	This function takes the key and table index and matches it with */
/*	the entries in the table . It returns a pointer to the matched  */
/*	element.							*/
/************************************************************************/

TBL_ELEMENT * match(key, tbl, ptr)
int key;
int tbl;
TBL_ELEMENT * ptr;
{
	while (ptr != NULL)
	{
		if ( (ptr->tbl == tbl) && (key == ptr->key))
				return(ptr);
		else
			ptr = ptr->next;
	}
	return(NULL);
}



/************************************************************************/
/*	This function packs the key, data etc for an entry 		*/
/************************************************************************/
char *
pack(operation, tbl, index, penum, key, data, size_data, entry,chareid, option)
char *operation, *tbl, *index;
int  penum;
char  *key;
char  *data;
char *size_data;
char *entry, *chareid;
char *option;
{
	int size, sized;
	int total_size;
	char *original, *ptr;
	TBL_MSG *msg;
	int size_chareid;

TRACE(CmiPrintf("[%d] Pack :: operation=%d, penum=%d, key=%d, entry=%d\n",
		 CmiMyPe(),
		 *((int *) operation), penum,
		 *((int *)key), *((int *)entry)));

	if (penum == CmiMyPe())
	{

TRACE(CmiPrintf("[%d] Pack :: operation=%d, penum=%d, key=%d, entry=%d\n",
		 CmiMyPe(),
		 *((int *) operation), penum,
		 *((int *)key), *((int *)entry)));

		msg = (TBL_MSG *) CkAllocMsg(sizeof(TBL_MSG));
		CkMemError(msg);
		msg->key = *((int *) key);
		msg->data = data;
		SendMsg(*((int *) entry), msg, (ChareIDType *) chareid);
	}
	else
	{

		size = sizeof(int);
		sized = *((int *) size_data);
		if (chareid == NULL)
			size_chareid = 0;
		else 
			size_chareid = SIZE_CHARE_ID;
		total_size = 9*size + sized + size_chareid;

TRACE(CmiPrintf("[%d] Pack:: size_chareid=%d, total_size=%d\n",
		CmiMyPe(), size_chareid, total_size));

		ptr = (char *) CkAllocMsg(total_size);
		CkMemError(ptr);
		original = ptr;
		structure_copy(ptr, operation, size);
		ptr += size;
		structure_copy(ptr, tbl, size);
		ptr += size;
		structure_copy(ptr, index, size);
		ptr += size;
		structure_copy(ptr, key, size);
		ptr += size;
		structure_copy(ptr, size_data, size);
		ptr += size;
		if (sized != 0)
		{
			structure_copy(ptr, data, sized);
			ptr += sized;
		}
		structure_copy(ptr, entry, size);
		ptr += size;
		structure_copy(ptr, &size_chareid, size);
		ptr += size;
		if (size_chareid != 0)
		{
			structure_copy(ptr, chareid, size_chareid);
			ptr += size_chareid;
		}
		structure_copy(ptr, option, size);
	TRACE(CmiPrintf("Pack :: sending key %d to penum %d\n", 
			*((int *) key), penum));
		GeneralSendMsgBranch(CsvAccess(CkEp_Tbl_Unpack), original,
				penum, ImmBocMsg, TblBocNum);
	}
}


/************************************************************************/
/*	This function intercepts local Insert calls, and then sends a 	*/
/*	message to the entry point BR_TblInsert on the processor on	*/
/*	which the element is to be stored with details about the element*/
/*	to be inserted.							*/
/************************************************************************/

TblInsert(tbl, index, key, data, size_data, entry, chareid, option)
int tbl;
int index;
int key;
char *data;
int size_data;
int entry;
ChareIDType *chareid;
int option;
{
	map *place;
	int operation = 0;
	ADDRESS *temp;
	TBL_ELEMENT *ptr;

	option = -1;
	if (index == -1)
		place = Hash(tbl, key);
	
	if (index == -1)
		trace_table(INSERT, tbl, key, place->penum);
	else 
		trace_table(INSERT, tbl, key, CmiMyPe());

	if (!data)  {
		CmiPrintf("*** ERROR *** Insert on processor %d has null data.\n",
					CmiMyPe());
		return;
	}
	if ( (index == -1) && (place->penum != CmiMyPe()))
		pack(&operation, &tbl, &(place->index), place->penum,
			 &key,  data, &size_data, &entry, chareid, &option);
	else
	{
		if (index == -1) index = place->index;
TRACE(CmiPrintf("TblInsert :: key = %d, index = %d\n", key, index));
		ptr = match(key, tbl, CpvAccess(table)[index]);
		operation = 3;
		if ( (ptr == NULL) ||  (! ptr->isDefined) )
		{
			if (ptr == NULL)
			{
				ptr = (TBL_ELEMENT *) CmiAlloc(sizeof(TBL_ELEMENT));
				CkMemError(ptr);
				ptr->tbl = tbl;
				ptr->key = key;
				ptr->reply = (ADDRESS *) NULL;
				ptr->next = CpvAccess(table)[index];
				CpvAccess(table)[index] = ptr;
TRACE(CmiPrintf("TblInsert :: table entry created with key %d\n", key));
			}		
			ptr->data = (char *) CmiAlloc(size_data);
			CkMemError(ptr->data);
			structure_copy(ptr->data, data, size_data);
			ptr->size_data = size_data;
			ptr->isDefined = 1;
			temp = ptr->reply;
			while (temp != NULL)
			{
TRACE(CmiPrintf("TblInsert :: Pending request key is %d  - data is %d\n",
		ptr->key, *((int *) data)));
				pack(&operation, &tbl, &index, GetID_onPE(temp->chareid),
						&ptr->key, ptr->data, &size_data,
						&temp->entry, &temp->chareid, &option);
				temp = temp->next;
			};
			if ((entry != -1) && (chareid != NULL))
				pack(&operation, &tbl, &index, GetID_onPE((*chareid)),
						&ptr->key, ptr->data, &size_data,  
						&entry, chareid, &option);
		}
		else
		{
			if ((entry != -1) && (chareid != NULL))
			{
				pack(&operation, &tbl, &index,
					GetID_onPE((*chareid)),
					 &ptr->key, ptr->data, &ptr->size_data,
					 &entry, chareid, &option);
			} 
		}
	}
}


/************************************************************************/
/*	This function intercepts local Delete calls, and then sends a	*/
/*	message to the entry point BR_TblDelete on the processor on	*/
/*	which the element is to be stored with details about the element*/
/*	to be deleted.							*/
/************************************************************************/

TblDelete(tbl, index, key, entry, chareid, option)
int tbl;
int index;
int key;
int entry;
ChareIDType *chareid;
int option;
{
	int size = 0;
	char *data = (char  *) NULL;
	map *place;
	int operation = 1;
	TBL_ELEMENT *ptr1, *ptr2;

	if ((option !=  TBL_REPLY) && (option != TBL_NOREPLY))
		CmiPrintf("***error*** TblDelete :: Unknown option chosen\n");
	if (index == -1)
		place = Hash(tbl, key);	

	if (index == -1)
		trace_table(DELETE, tbl, key, place->penum);
	else 
		trace_table(DELETE, tbl, key, CmiMyPe());

	if ( (index == -1) && ( place->penum != CmiMyPe()))
		pack(&operation, &tbl, &(place->index), place->penum,
			 &key, data, &size,  &entry, chareid, &option);
	else
	{
		if (index == -1) index  = place->index;
		ptr1 = CpvAccess(table)[index];
		ptr2 = (TBL_ELEMENT *) NULL;
		while ( (ptr1 != NULL) &&
			(  (ptr1->tbl != tbl) || 
			   (key != ptr1->key)))
		{
			ptr2 = ptr1;
			ptr1 = ptr1->next;
		}
		operation = 3;
		if ( (ptr1 != NULL) && (entry != -1) && (chareid != NULL)) 
			pack(&operation, &tbl, &index, 
				GetID_onPE((*chareid)),
				&ptr1->key, ptr1->data, &ptr1->size_data,
				&entry, chareid, &option);
		if ( (ptr1 == NULL) && (entry != -1) && (chareid != NULL) 
				    && (option == TBL_REPLY))
			pack(&operation, &tbl, &index, GetID_onPE((*chareid)),
				 &key, data, &size, &entry, chareid, &option);
		if (ptr2 == NULL) 
			if (ptr1 != NULL)
			{
				CpvAccess(table)[index] =  ptr1->next;
				CmiFree(ptr1);
			}
			else
				CpvAccess(table)[index] = (TBL_ELEMENT *) NULL;
		else
			if (ptr1 != NULL)
			{
				ptr2->next = ptr1->next;
				CmiFree(ptr1);
			}
			else
				ptr2->next =(TBL_ELEMENT *) NULL;
	}	
}


/************************************************************************/
/*	This function intercepts local Find calls, and then sends a 	*/
/* 	message to the entry point BR_Tbl_Find on the processor on which*/
/* 	the element may exist with information about element to be found*/
/************************************************************************/

TblFind( tbl, index, key, entry, chareid, option)
int tbl;
int index;
int key;
int entry;
ChareIDType *chareid;
int option;
{
	char * data = (char *) NULL;
	int size =  0;
	map *place;
	int operation = 2;
	TBL_ELEMENT *ptr;
	ADDRESS *temp;

	if ((option != TBL_NEVER_WAIT) && (option != TBL_ALWAYS_WAIT)
		&& (option != TBL_WAIT_AFTER_FIRST))
			CmiPrintf("***error*** TblFind :: Unknown option chosen\n");
	place = Hash(tbl, key);	

	if (index == -1)
		trace_table(FIND, tbl, key, place->penum);
	else 
		trace_table(FIND, tbl, key, CmiMyPe());


	if ( (index == -1) && (place->penum != CmiMyPe()))
		pack(&operation, &tbl, &(place->index), place->penum,
			 &key, data, &size, &entry, chareid, &option);
	else
	{
		if (index == -1) index = place->index;
		ptr = match(key, tbl, CpvAccess(table)[index]);	

TRACE(CmiPrintf("[%d] TblFind: ptr=0x%x, entry=%d, option=%d, index=%d\n",
		CmiMyPe(), ptr, entry, option, index));

		operation = 3;
		if (ptr != NULL)
			if (ptr->isDefined)
				{
				if ((entry != -1) && (chareid != NULL))
					pack(&operation, &tbl, &index, 
						GetID_onPE((*chareid)), &key, 
						ptr->data, &ptr->size_data,
						&entry, chareid, &option);
				}
			else
			{
			   if (option == TBL_NEVER_WAIT)
			   {
				if ((entry != -1) && (chareid != NULL))
					pack(&operation, &tbl, &index, 
						GetID_onPE((*chareid)), &key,
						data, &size,
						&entry, chareid, &option);
			   }
			   else
			   {
				if ((entry != -1) && (chareid != NULL))
				{
				   temp = (ADDRESS *) CmiAlloc(sizeof(ADDRESS));
				   CkMemError(temp);
				   temp->entry = entry;
				   temp->chareid = *chareid;
				   temp->next = ptr->reply;
				   ptr->reply = temp;
				}
			   }
			}
		else
		{
TRACE(CmiPrintf("[%d TblFind: ptr is NULL, tbl=%d, key=%d\n",
	CmiMyPe(), tbl, key));

			ptr = (TBL_ELEMENT *) CmiAlloc(sizeof(TBL_ELEMENT));
TRACE(CmiPrintf("[%d] TblFind: Going to send message.\n"));
			CkMemError(ptr);
			ptr->tbl = tbl;
			ptr->key = key;
			ptr->reply = (ADDRESS *) NULL;
			ptr->next = CpvAccess(table)[index];
			CpvAccess(table)[index] = ptr;
			ptr->isDefined = 0;
			ptr->data = (char *) NULL;
			ptr->size_data = 0;

			if (option == TBL_ALWAYS_WAIT)
			{
			   if ((entry != -1) && (chareid != NULL)) 
			   {
				temp = (ADDRESS *) CmiAlloc(sizeof(ADDRESS));
				CkMemError(temp);
				temp->entry = entry;
				temp->chareid = *chareid;
				temp->next = ptr->reply;
				ptr->reply = temp;
			   }
			}
			else
			   if ((entry != -1) && (chareid != NULL)) 
				pack(&operation, &tbl, &index, 
					GetID_onPE((*chareid)),
					&key, data, &size, &entry, chareid,
					&option);
TRACE(CmiPrintf("[%d] TblFind: Sent message.\n"));

		}
	}
}

TblAddSysBocEps()
{
  CsvAccess(CkEp_Tbl_Unpack) =
    registerBocEp("CkEp_Tbl_Unpack",
		  Unpack,
		  CHARM, 0, 0);
}
