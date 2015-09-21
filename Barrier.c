#include <linux/slab.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/wait.h>
#include <linux/kernel.h>
#include <linux/linkage.h>

#include "HashTable.h"

/* *********************************************************************
 * EXCL:  installazioni esclusive 
 * NEXCL: installazioni non esclusive
 * READ:  semplici aperture
 * ********************************************************************/
#define EXCL	0
#define NEXCL	1
#define READ	2

/* *********************************************************************
 * Definizione dell'intervallo TAG [0,31] 
 * ********************************************************************/
#define TAG_MIN 0
#define TAG_MAX 32

/* *********************************************************************
 * Struttura globale per il project_a
 * puntatore alla struttura HashTable  
 * ********************************************************************/
HashTable* global_hashtable;

int HashTable_RemoveElem(HashTable* hashtable, int bd, int tag);		
int HashTable_Remove(HashTable* hashtable, int bd);
int HashTable_Exist(HashTable* hashtable, key_t key);
HashTable* create(void);

/* *********************************************************************
 * Funzione di inizializzazione del project_a
 * invoca dalla prima chiamata ad una get_barrier
 * ********************************************************************/
void project_a_init(void) {
	printk(KERN_INFO "Inizializzo le strutture dati...\n");
	global_hashtable = HashTable_Create();
	if(global_hashtable != NULL) 
		printk(KERN_INFO " Strutture dati inizializzate correttamente\n");
	else printk(KERN_INFO "Errore nella chiamata della init\n");
}

/* *********************************************************************
 * Controllo intervallo TAG
 * -1 in caso di TAG errato
 * ********************************************************************/
int check_tag(int tag) {
	if(tag >= TAG_MIN && tag < TAG_MAX) 
		return 1;
	else {
		printk(KERN_INFO "Project_a: Errore, valore TAG errato\n");
		return -1;
	}
}

/* *********************************************************************
 * Installazione della barriera con chiave specifica key_t key
 * flag specifica installazioni esclusive o non, o semplici aperture
 * la funzione ritorna il barrier descriptor.
 * Se il flag non è impostato viene ritornato -1.
 * Tipologia di flag:
 *   - EXCL:  Se esiste già una barriera installata torna -1
 *   - NEXCL: Torna il barrier descriptor se la barriera è già presente
 *            oppure installa la barriera.
 *   - READ:  Torna un codice operativo se la barriera esiste
 * ********************************************************************/
asmlinkage int sys_get_barrier(key_t key, int flag){

	int bd;
	int exist;
	
	if(global_hashtable == NULL)
		project_a_init();
	 
	if(global_hashtable != NULL) {
		if(flag == READ){
			exist = HashTable_Exist(global_hashtable, key);			
			if(exist != -1){
				return exist;
			}
			else return -1;
		}
		if(flag == EXCL){
			bd = HashTable_Insert(global_hashtable, key, EXCL);
			return bd;
		}
		if(flag == NEXCL){
			bd = HashTable_Insert(global_hashtable, key, NEXCL);
			return bd;
		}
		if(flag == 3){
			HashTable_toString(global_hashtable);
		}

	}
	return -1;
} 

/* *********************************************************************
 * Sleep sulla barriera indicata dal barrier descriptor con TAG relativo
 * La funzione ritorna -1 in caso di errore oppure 0.
 * ********************************************************************/
asmlinkage int sys_sleep_on_barrier(int bd, int tag){

	HashTable* hash = global_hashtable;	
	int tag_correct = check_tag(tag);
	if(hash != NULL && tag_correct == 1)
		return HashTable_InsertElem(hash, bd, tag);
	else return -1;

} 

/* *********************************************************************
 * Risveglio barriera indicata dal barrier descriptor con TAG relativo
 * La funzione ritorna -1 in caso di errore oppure 0.
 * ********************************************************************/
asmlinkage int sys_awake_barrier(int bd, int tag){

	HashTable* hash = global_hashtable;	
	int tag_correct = check_tag(tag);
	if(hash != NULL && tag_correct == 1) {
		return HashTable_RemoveElem(hash, bd, tag);		
	} 
	else return -1; 
	
} 

/* *********************************************************************
 * Disinstallazione della barriera specificata dal barrier descriptor
 * La funzione ritorna -1 nel caso non sia presente la barriera oppure
 * 1 in caso di rimozione eseguita con successo
 * ********************************************************************/
asmlinkage int sys_release_barrier(int bd){
	
	HashTable* hash = global_hashtable;
	if(hash != NULL) {
		return HashTable_Remove(hash, bd);
	}
	else return -1;
	
}
