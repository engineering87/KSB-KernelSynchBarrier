#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <asm/spinlock.h>   
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/spinlock_types.h>

#include "HashTable.h"


/* *********************************************************************
 * Create della tabella hash con le relative strutture dati
 * ********************************************************************/
HashTable*  HashTable_Create(void){
	
	int i;
	int bucket_correct = 0;	
	HashTable* hashtable = kmalloc(sizeof(HashTable), GFP_KERNEL);
	
	if(hashtable){
		for(i = 0; i < HASHSIZE; i++){

			hashtable->bucket[i] = kmalloc(sizeof(Hash_Table_Bucket), GFP_KERNEL);
			if(hashtable->bucket[i]) {
				bucket_correct++;
				mutex_init(&(hashtable->bucket[i]->bucket_lock));
				hashtable->bucket[i]->head = NULL;
			}
			
		}
	}
	if(bucket_correct == (HASHSIZE))
		return hashtable; 
	else {
		for(i = 0; i< HASHSIZE; i++) 
			kfree(hashtable->bucket[i]);
		kfree(hashtable);
		return NULL;
	}
}

/* *********************************************************************
 * HashTable_Insert trasforma la chiave key attraverso transform_Key
 * in un barrierid e calcola attraverso deterministic_Function il bucket
 * in cui depositare la chiave ed il barrierid che sarà restituito
 * all'utente.
 * ********************************************************************/
int HashTable_Insert(HashTable* hashtable, key_t key, int flag){

	int exist;
	int barrierid;
	int numberbucket;
	Hash_Table_Elem* elem;

	barrierid    = transform_Key(key);
	numberbucket = deterministic_Function(barrierid);

/* *********************************************************************
 * Se il bucket è vuoto aggiungi semplicemente l'elemento al bucket,
 * altrimenti controlla che gli altri elementi presenti nel bucket
 * abbiano chiave diversa.
 * Se la chiave è diversa aggiungi l'elemento alla lista,
 * altrimenti restituisci il barrierid dell'Hash_Table_Elem già immesso.
 * -1 viene ritornato se la barriera esiste e ha un'installazione
 * esclusiva.
 * ********************************************************************/

	mutex_lock(&(hashtable->bucket[numberbucket]->bucket_lock));
	
	if(hashtable->bucket[numberbucket]->head == NULL){
		elem = crea_Elem(key, barrierid, flag);
		hashtable->bucket[numberbucket]->head = elem;
	}
	else{
		exist = elem_Exist(hashtable->bucket[numberbucket]->head, key);

		if(exist == 2 || (exist == 1 && flag == 0)){
			mutex_unlock(&(hashtable->bucket[numberbucket]->bucket_lock));
			return -1;
		}
		if(exist == 1){
			mutex_unlock(&(hashtable->bucket[numberbucket]->bucket_lock));
			return barrierid;
		}

		elem = crea_Elem(key, barrierid, flag);
		
		if(elem) {
			elem->next = hashtable->bucket[numberbucket]->head;
			hashtable->bucket[numberbucket]->head = elem;
		}

	}

	mutex_unlock(&(hashtable->bucket[numberbucket]->bucket_lock));
	return barrierid;
}

Hash_Table_Elem* crea_Elem(key_t key, int barrierid, int flag){
	
	int i;
	Hash_Table_Elem* elem;
	
	elem = (Hash_Table_Elem*)kmalloc(sizeof(Hash_Table_Elem), GFP_KERNEL);
	
	if(elem) {

		if(!flag)
			elem->exclusive     = 1;
		else
			elem->exclusive     = 0;
		elem->key               = key;
		elem->next              = NULL;
		elem->barrierdescriptor = barrierid;

		for(i = 0; i < 32; i++){
			elem->occupied[i] = 0;
			init_waitqueue_head(&(elem->nodo[i]));
		}
		
	}
	return elem;
}

/* *********************************************************************
 * Funzione utente che notifica all'utente se esiste una barriera
 * installata con chiave key.
 * La funzione torna -1 se non viene trovata la barriera altrimenti
 * torna il barrier-descriptor.
 * ********************************************************************/
int HashTable_Exist(HashTable* hashtable, key_t key){
	
	int bd;
	int index;
	Hash_Table_Elem* elem;

	bd    = transform_Key(key);
	index = deterministic_Function(bd);
	elem  = hashtable->bucket[index]->head;

	if(!elem)
		return -1;
	while(elem){
		if(elem->key == key && elem->barrierdescriptor == bd)
			return bd;
		elem = elem->next;
	}
	return -1;
}

/* *********************************************************************
 * Rimozione dell'elemento con barrier descriptor bd.
 * Dal bd ricavo il bucket in cui spiazzarmi e da lì controllo tramite
 * due puntatori dove si trova l'elemento e lo taglio dalla lista.
 * ********************************************************************/
int HashTable_Remove(HashTable* hashtable, int bd){

	int index;
	int empty;
	Hash_Table_Elem* aux;
	Hash_Table_Elem* elem;

	index = deterministic_Function(bd);
	
	mutex_lock(&(hashtable->bucket[index]->bucket_lock));

	aux   = hashtable->bucket[index]->head;
	elem  = hashtable->bucket[index]->head;
	
	if(!elem) {
		mutex_unlock(&(hashtable->bucket[index]->bucket_lock));
		return -1;
	}

	if(elem->barrierdescriptor == bd){		
		empty = checkempty(elem);

		if(!empty){
			mutex_unlock(&(hashtable->bucket[index]->bucket_lock));
			return -1;
		}
		hashtable->bucket[index]->head = elem->next;
		kfree(elem);
		elem = NULL;
		
		mutex_unlock(&(hashtable->bucket[index]->bucket_lock));
		return 1;
	}
	else{
		elem = elem->next;
		while(elem){
			if(elem->barrierdescriptor == bd){
				empty = checkempty(elem);				
				if(!empty){
					mutex_unlock(&(hashtable->bucket[index]->bucket_lock));
					return -1;
				}
				aux->next = elem->next;
				kfree(elem);
				elem = NULL;
			}
			else{
				aux  = elem;
				elem = elem->next;
			}
		}
		mutex_unlock(&(hashtable->bucket[index]->bucket_lock));
		return 1;
	}
	mutex_unlock(&(hashtable->bucket[index]->bucket_lock));
	return -1;
}

//torna 1 se è vuota 0 altrimenti
int checkempty(Hash_Table_Elem* elem){
	int i;
	
	for(i = 0; i < 32; i++){
		if(elem->occupied[i])
			return 0;
	}
	return 1;
}

int HashTable_InsertElem(HashTable* hash, int bd, int tag){

	struct Hash_Table_Elem* elem;
	int exist;
	int numberbucket;
	
	numberbucket = deterministic_Function(bd);
	
	mutex_lock(&(hash->bucket[numberbucket]->bucket_lock));

	exist = barrier_Exist(hash, bd);
	if(!exist) {
		mutex_unlock(&(hash->bucket[numberbucket]->bucket_lock));
		return -1;
	}
	
	elem = hash->bucket[numberbucket]->head;
	
	while(elem && elem->barrierdescriptor != bd){
		elem = elem->next;
	}

	elem->occupied[tag] = 1;		
	mutex_unlock(&(hash->bucket[numberbucket]->bucket_lock));
	interruptible_sleep_on(&(elem->nodo[tag]));		
		
	return 1;

}

int HashTable_RemoveElem(HashTable* hash, int bd, int tag){

	Hash_Table_Elem* elem;
	int exist;
	int numberbucket;
	
	numberbucket = deterministic_Function(bd);
	
	mutex_lock(&(hash->bucket[numberbucket]->bucket_lock));

	exist = barrier_Exist(hash, bd);
	if(!exist) {
		mutex_unlock(&(hash->bucket[numberbucket]->bucket_lock));
		return -1;
	}

	elem = hash->bucket[numberbucket]->head;

	while(elem && elem->barrierdescriptor != bd){
		elem = elem->next;
	}

	elem->occupied[tag] = 0;
	wake_up(&(elem->nodo[tag]));
	mutex_unlock(&(hash->bucket[numberbucket]->bucket_lock));	

	return 1;
}
/* *********************************************************************
 * Stampa degli elementi all'interno della hashtable
 * ********************************************************************/
void HashTable_toString(HashTable* hashtable){

	int i;
	for(i = 0; i < HASHSIZE; i++){
		if(!hashtable->bucket[i]){
			printk("elementi nel bucket %d assenti\n", i);
		}
		else{
			Hash_Table_Elem* elem = hashtable->bucket[i]->head;
			printk("elementi nel bucket: %d\n", i);

			while(elem){
				printk("	chiave: %d		barrier-descriptor assegnato: %d\n", elem->key, elem->barrierdescriptor);
				elem = elem->next;
			}
		}
	}
}

/* *********************************************************************
 * Funzione deterministica che a partire da un barrier descriptor crea
 * un indice che sarà lo spiazzamento all'interno dell'hashtable
 * ********************************************************************/
int deterministic_Function(int bd){
	return bd%HASHSIZE;
}

/* *********************************************************************
 * Associo ad una chiave univoca un intero univoco, banalmente ora torno
 * la chiave stessa univoca per definizione.
 * ********************************************************************/
int transform_Key(key_t key){
	return key;
}

/* *********************************************************************
 * Funzione di servizio interna.
 * A partire da elem controllo che esista già un elemento collegato alla
 * lista con chiave key.
 * La funzione ritorna:
 * 2 nel caso in cui la chiave sia presente con installazione esclusiva
 * 1 nel caso in cui la chiave sia presente
 * 0 nel caso in cui la chiave non sia presente
 * ********************************************************************/
int elem_Exist(Hash_Table_Elem* elem, key_t key){
	
	Hash_Table_Elem* aux;
	aux = elem;
	while(aux){
		if(aux->key == key){
			if(aux->exclusive)
				return 2;
			return 1;
		}
		aux = aux->next;
	}
	return 0;
}

//torna 1 se esiste una barriera con barriera bd oppure 0
int barrier_Exist(HashTable* hash, int bd){

	int numberbucket;
	Hash_Table_Elem* elem;

	numberbucket = deterministic_Function(bd);
	
	elem = hash->bucket[numberbucket]->head;
	
	if(!elem) 
		return 0;	

	while(elem){
		if(elem->barrierdescriptor == bd)
			return 1;
		elem = elem->next;
	}
	
	return 0;
}
