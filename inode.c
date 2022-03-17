#include "allocation.h"
#include "inode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BLOCKSIZE 4096

// vi bruker en static int for aa gi ting id
static int current_id = 0;

struct inode* create_file( struct inode* parent, char* name, char readonly, int size_in_bytes )
{
	if (!parent) {
		return NULL;
	}
	else if (!(parent->is_directory) || find_inode_by_name(parent,name)) {
		return NULL;
	}

	// kom vi hit er vi good
	struct inode* new_file = (struct inode*) malloc(sizeof(struct inode));
	if (!new_file) {
		fprintf(stderr, "Malloc failed. Aborting.\n");
		exit(EXIT_FAILURE);
	}

	new_file->id = current_id++;
	new_file->name = strdup(name);
	new_file->is_directory = 0;
	new_file->is_readonly = readonly;
	new_file->filesize = size_in_bytes;

	// de to siste attributtene maa vi lage utifra filesize

	int blocks_required = size_in_bytes/BLOCKSIZE;
	// denne vil alltid runde ned siden det er en int, saa vi
	// maa plusse paa 1 dersom oppdelingen ikke gaar eksakt opp
	if (!(size_in_bytes%BLOCKSIZE)) blocks_required++;

	size_t* entries = malloc(sizeof(size_t)*blocks_required);

	for (int i = 0; i < blocks_required; i++) {
		// sjekka litt og det virker som man ikke trenger
		// eksplisitt casting selvom en int er 4 bytes og
		// en size_t 8 paa min pc
		entries[i] = allocate_block();
	}

	new_file->num_entries = blocks_required;
	new_file->entries = entries;

	// naa har vi lagd new_file, men vi maa putte den i parent sine
	// entries. Saa vidt jeg kan tenke meg maa vi nok bare
	// kopiere alle entries i et nytt array som er en stoerre ogsaa 
	// free'e gamle entries siden det bare er et array og ingen lenkeliste
	// eller noe lignende
	size_t* new_entries = (size_t*) malloc(sizeof(size_t)*(parent->num_entries+1));
	// dersom den er null setter vi bare pointern til new entries
	if (!(parent->num_entries)) {
		parent->entries = new_entries;
	}
	else {
		memcpy(new_entries,parent->entries,parent->num_entries*sizeof(size_t));
		free(parent->entries);
		parent->entries = new_entries;
	}

	// vi har naa kopiert over minnet, det siste vi maa gjoere er 
	// aa ta det nye directoriet aa putte det paa plasse length-1.
	// Dette er tilfeldigvis num_entries fordi den ikke har blitt
	// oppdatert enda
	parent->entries[parent->num_entries] = new_file;

	parent->num_entries++;

	return new_file;

}


struct inode* create_dir( struct inode* parent, char* name )
{
	// foerst maa vi sjekke om parent er null, hvis den er det
	// er det root det er snakk om her
	if (!parent) {
		// da er det lett vi bare lager en inode og trenger ikke
		// putte den noe spesifikt sted
		struct inode* new_dir = (struct inode*) malloc(sizeof(struct inode));
		if (!new_dir) {
			fprintf(stderr, "Malloc failed. Aborting.\n");
			exit(EXIT_FAILURE);
		}

		new_dir->id = current_id++;
		new_dir->name = strdup(name);
		if (!new_dir->name) {
			fprintf(stderr, "Strdup alloc failed. Aborting.\n");
			exit(EXIT_FAILURE);
		}
		new_dir->is_directory = 1;
		new_dir->is_readonly = 0;
		new_dir->filesize = 0;
		new_dir->num_entries = 0;
		new_dir->entries = NULL;

		return new_dir;
	}
	// ellers maa vi legge det nye directoriet under parent
	else {
		if (!(parent->is_directory)) return NULL;
		// dersom en fil med samme navn ogsaa fins boer vi heller ikke
		// lage den, i foelge carsten paa mattermost
		if (find_inode_by_name(parent,name)) return NULL;

		struct inode* new_dir = (struct inode*) malloc(sizeof(struct inode));
		if (!new_dir) {
			fprintf(stderr, "Malloc failed. Aborting.\n");
			exit(EXIT_FAILURE);
		}

		new_dir->id = current_id++;
		new_dir->name = strdup(name);
		new_dir->is_directory = 1;
		new_dir->is_readonly = 0;
		new_dir->filesize = 0;
		new_dir->num_entries = 0;
		new_dir->entries = NULL;

		// naa har vi lagd new_dir, men vi maa putte den i parent sine
		// entries. Saa vidt jeg kan tenke meg maa vi nok bare
		// kopiere alle entries i et nytt array som er en stoerre ogsaa 
		// free'e gamle entries siden det bare er et array og ingen lenkeliste
		// eller noe lignende
		size_t* new_entries = (size_t*) malloc(sizeof(size_t)*(parent->num_entries+1));
		// dersom den er null setter vi bare pointern til new entries
		if (!(parent->num_entries)) {
			parent->entries = new_entries;
		}
		else {
			memcpy(new_entries,parent->entries,parent->num_entries*sizeof(size_t));
			free(parent->entries);
			parent->entries = new_entries;
		}

		// vi har naa kopiert over minnet, det siste vi maa gjoere er 
		// aa ta det nye directoriet aa putte det paa plasse length-1.
		// Dette er tilfeldigvis num_entries fordi den ikke har blitt
		// oppdatert enda
		parent->entries[parent->num_entries] = new_dir;

		parent->num_entries++;

		return new_dir;
	}
}

struct inode* find_inode_by_name( struct inode* parent, char* name )
{
	if (!parent->is_directory) {
		printf("Invalid: %s is not a directory.\n", parent->name);
		return NULL;
	}

	for (int i = 0; i < parent->num_entries; i++) {
		struct inode* child = (struct inode*) parent->entries[i];
		if (strcmp(child->name,name) == 0) {
			struct inode* match = (struct inode*) parent->entries[i];
			return match;
		}
	}
    return NULL;
}

// rekursiv hjelpefunksjon som brukes i load_inodes
struct inode *load_next(FILE *file) {
	int id;
	// det er greit aa hardcode lesing av fire bytes fordi superblokken
	// alltid vil ha det
	int ret = fread(&id, 4, 1, file);

	// her bare antar jeg at ingen feil oppstaar og hver ret=0
	// er eof. Dersom superblokken er velformatert vil dette aldri
	// skje uansett
	if (ret == 0) {
		printf("End of file.\n");
		return NULL;
	}

	int name_len;
	// litt weak error reporting men gaar nok bra
	// Siden vi bare leser ett element returnerer denne
	// bare 0 eller 1
	if (!(fread(&name_len,4,1,file))) exit(EXIT_FAILURE);

	char* name = (char*) malloc(sizeof(char)*name_len);
	if (!name) {
		fprintf(stderr, "Malloc failed. Aborting.\n");
		exit(EXIT_FAILURE);
	}

	if(!(fread(name,name_len*sizeof(char),1,file))) exit(EXIT_FAILURE);

	char is_directory;
	if(!(fread(&is_directory,1,1,file))) exit(EXIT_FAILURE);

	char is_readonly;
	if (!(fread(&is_readonly,1,1,file))) exit(EXIT_FAILURE);

	int filesize;
	if (!(fread(&filesize,4,1,file))) exit(EXIT_FAILURE);

	int num_entries;
	if (!(fread(&num_entries,4,1,file))) exit(EXIT_FAILURE);

	size_t* entries = (size_t*) malloc(sizeof(size_t)*num_entries);
	if (!entries) {
		fprintf(stderr, "Malloc failed. Aborting.\n");
		exit(EXIT_FAILURE);
	}

	struct inode* inode = malloc(sizeof(struct inode));
	if (!inode) {
		fprintf(stderr, "Malloc failed. Aborting.\n");
		exit(EXIT_FAILURE);
	}

	if (is_directory) {
		// her skjer rekursjonen
		// vi ser at superblokken er skrevet paa en logisk maate:
		// dersom root->entries har inoder med id 1,2,4 og 6 er 
		// dette rekkefoelgen de er i superblokken (med deres barn
		// imellom). Derfor trenger vi ikke aa passe paa at riktig
		// inode kommmer paa riktig plass i entries, vi kan bare
		// plassere neste inode i minnet paa neste plass i entries

		// men det foerste vi maa gjoere er aa flytte file position indicatoren
		// til starten av neste inode, ettersom vi egentlig ikke bryr oss om
		// oppsettet til entries paa disk
		fseek(file,sizeof(size_t)*num_entries,SEEK_CUR);

		for (int i = 0; i < num_entries; i++) {
			entries[i] = load_next(file);
		}
	}
	// dersom vi ikke har et directory bryr vi oss faktisk om entries,
	// da de peker paa diskblokker
	else {
		for (int i = 0; i < num_entries; i++) {
			size_t entry;
			fread(&entry,sizeof(size_t),1,file);
			entries[i] = entry;
		}
		// naar vi er ferdig med denne burde SEEK_CUR vaere paa riktig
		// sted
	}

	inode->id = id;
	inode->name = name;
	inode->is_directory = is_directory;
	inode->is_readonly = is_readonly;
	inode->filesize = filesize;
	inode->num_entries = num_entries;
	inode->entries = entries;

	// printf("ID: %d\n", inode->id);
	// printf("NAME: %s\n", inode->name);
	// printf("directory: %d\n", inode->is_directory);

	return inode;
}
// denne boer nok returnere root sin i-node vil jeg anta
struct inode* load_inodes()
{
	FILE* file = fopen("superblock", "r");

	if (!file) {
		perror("file");
		exit(EXIT_FAILURE);
	}

	// superblokkene ser ut til aa vaere formatert paa maaten man vil
	// forvente (maaten ting blir printa ut med tree for eksempel), 
	// nemlig at etter hver inode vil dens barn i filtreet etterfoelge,
	// og dersom et av barna er et nytt directory vil dens filer foelge
	// foer vi gaar videre til neste fil i forfeldredirectoriet.
	// dette betyr at vi kan bruke rekursjon til aa lese inn inodene
	struct inode* root = load_next(file);
	fclose(file);
	return root;
}

// rekursiv hjelpefunksjon for fs_shutdown

int free_self(struct inode* self /* tror ikke self er et keyword i c */) {
	if (!self) return 0;

	if (!(self->is_directory)) {
		free(self->entries);
		free(self->name);
		free(self);
		return 0;
	}
	else {
		for (int i = 0; i < self->num_entries; i++) {
			struct inode* barn = (struct inode*) self->entries[i];
			free_self(barn);
		}
		free(self->entries);
		free(self->name);
		free(self);
	}
}

void fs_shutdown( struct inode* inode )
{
	// strengt tatt kunne alt i free_self() bli putta her
	// men jeg tenker det er rart og kalle fs_shutdown paa
	// bare en fil i rekursjonen.
	free_self(inode);
}

/* This static variable is used to change the indentation while debug_fs
 * is walking through the tree of inodes and prints information.
 */
static int indent = 0;

void debug_fs( struct inode* node )
{
    if( node == NULL ) return;
    for( int i=0; i<indent; i++ )
        printf("  ");
    if( node->is_directory )
    {
        printf("%s (id %d)\n", node->name, node->id );
        indent++;
        for( int i=0; i<node->num_entries; i++ )
        {
            struct inode* child = (struct inode*)node->entries[i];
            debug_fs( child );
        }
        indent--;
    }
    else
    {
        printf("%s (id %d size %db blocks ", node->name, node->id, node->filesize );
        for( int i=0; i<node->num_entries; i++ )
        {
            printf("%d ", (int)node->entries[i]);
        }
        printf(")\n");
    }
}

