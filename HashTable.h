#ifndef __HashTable__
#define __HashTable__

#define HASHSIZE 128
#define BARRIERSIZE 32

typedef struct HashTable HashTable;
typedef struct Hash_Table_Bucket Hash_Table_Bucket;
typedef struct Hash_Table_Elem Hash_Table_Elem;
typedef struct list_head list_head;

/* *********************************************************************
 * Strutture dati:
 * HashTable
 * 		array di puntatori a Hash_Table_Elem di dimensione HASHSIZE
 * 
 * Hash_Table_Elem
 * 		chiave               key_t fornita dal programmatore per 
 *                           sincronizzarsi
 * 		barrierdescriptor    creato a runtime in maniera univoca
 * 		next                 puntatore all'elemento successivo
 * ********************************************************************/
struct HashTable{
	
	Hash_Table_Bucket* bucket[HASHSIZE];

};

struct Hash_Table_Bucket{
	struct mutex bucket_lock;
	//spinlock_t spinlock;
	Hash_Table_Elem* head;
};

struct Hash_Table_Elem{
	int exclusive;
	key_t key;
	int barrierdescriptor;
	int occupied[BARRIERSIZE];
	wait_queue_head_t nodo[BARRIERSIZE];
	struct Hash_Table_Elem* next;

};

/* *********************************************************************
 * Funzioni "pubbliche"
 * HashTable_Create:   Istanzia l'HashTable e torna il puntatore
 *                     all'hash appena creato.
 * HashTable_Insert:   Installa la barriera con chiave key nell' hash.
 * HashTable_Exist:    Verifica che esista una barriera con chiave key
 *                     nell'hash.
 * HashTable_Remove:   Disinstalla la barriera bd dalla tabella hash.
 * HashTable_toString: Stampa l'intera tabella hash.
 * ********************************************************************/
HashTable*  	 HashTable_Create(void);
int         	 HashTable_Insert(HashTable* hash, key_t key, int flag);
int        	 	 HashTable_Exist(HashTable* hash, key_t key);
int         	 HashTable_Remove(HashTable* hash, int bd);
void    	     HashTable_toString(HashTable* hash);
int				 HashTable_InsertElem(HashTable* hash, int bd, int tag);
Hash_Table_Elem* crea_Elem(key_t key, int barrierid, int flag);

/* *********************************************************************
 * Funzioni ausiliarie
 * deterministic_Function: Crea da una bd l'indice della tabella in cui
 *                         posizionare l'elemento
 * transform_key:          Da una chiave univoca crea un indice intero
 *                         univoco che diverrà il barrier descriptor.
 * elem_Exist:             Notifica se nella lista a partire da elem
 *                         è presente un Hash_Table_Elem con chiave key.
 * ********************************************************************/
int         deterministic_Function(int barrierdescriptor);
int         transform_Key(key_t key);
int         elem_Exist(Hash_Table_Elem* elem, key_t key);
int 		checkempty(Hash_Table_Elem* elem);
int 		barrier_Exist(HashTable* hash, int bd);

//HashTable*  get_pointer_hashtable(void);

#endif
