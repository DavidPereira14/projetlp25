#include "deduplication.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/md5.h>
#include <dirent.h>

// Fonction de hachage MD5 pour l'indexation
// dans la table de hachage
unsigned int hash_md5(unsigned char *md5) {
    unsigned int hash = 0;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        hash = (hash << 5) + hash + md5[i];
    }
    return hash % HASH_TABLE_SIZE;
}

// Fonction pour calculer le MD5 d'un chunk
void compute_md5(void *data, size_t len, unsigned char *md5_out) {
    MD5_CTX md5_ctx;

    // Initialisation du contexte MD5
    MD5_Init(&md5_ctx);

    // Mise à jour du contexte avec les données
    MD5_Update(&md5_ctx, data, len);

    // Finalisation du calcul du MD5 et stockage du résultat dans md5_out
    MD5_Final(md5_out, &md5_ctx);
}

// Fonction permettant de chercher un MD5 dans la table de hachage
int find_md5(Md5Entry *hash_table, unsigned char *md5) {
    /* @param: hash_table est le tableau de hachage qui contient les MD5 et l'index des chunks unique
    *           md5 est le md5 du chunk dont on veut déterminer l'unicité
    *  @return: retourne l'index s'il trouve le md5 dans le tableau et -1 sinon
    */
    for(int i = 0;i<HASH_TABLE_SIZE;i++) {
        if (memcmp(hash_table[i].md5,md5,HASH_TABLE_SIZE) == 0){
            return hash_table[i].index;
        }
    }
    return -1;
}

// Ajouter un MD5 dans la table de hachage
void add_md5(Md5Entry *hash_table, unsigned char *md5, int index) {
    unsigned int position = hash_md5(md5);
    while(hash_table[position].index == -1){
        position = (position + 1) % HASH_TABLE_SIZE;
    }
    memcpy(hash_table[position].md5,md5,MD5_DIGEST_LENGTH);
    hash_table[position].index = index;
}

// Fonction pour convertir un fichier non dédupliqué en tableau de chunks
void deduplicate_file(FILE *file, Chunk *chunks, Md5Entry *hash_table) {
    /* @param:  file est le fichier qui sera dédupliqué
    *           chunks est le tableau de chunks initialisés qui contiendra les chunks issu du fichier
    *           hash_table est le tableau de hachage qui contient les MD5 et l'index des chunks unique
    */
    unsigned char buffer[CHUNK_SIZE];
    int chunk_index = 0;
    size_t octets_lu;
    unsigned char md5[MD5_DIGEST_LENGTH];
    //Lecture du fichier chunk par chunk
    while ((octets_lu = fread(buffer,1,CHUNK_SIZE,file)) > 0) {
        //On calcule le MD5 du chunck lu
        compute_md5(buffer,octets_lu,md5);

        //Verification que ce MD5 est dans la table de hachage, Si le MD5 n'existe pas encore, l'ajouter à la table de hachage et au tableau de chunks
        int existing_index = find_md5(hash_table,md5);
        if (existing_index == -1) {
            chunks[chunk_index].data = malloc(octets_lu);
            if (!chunks[chunk_index].data) {
                perror("Erreur d'allocation mémore");
                exit(EXIT_FAILURE);
            }
            memcpy(chunks[chunk_index].data,buffer,octets_lu); //On copie les données dans le chunk
            memcpy(chunks[chunk_index].md5,md5,MD5_DIGEST_LENGTH); //On copie le MD5 dans la structure
            add_md5(hash_table,md5,chunk_index);//On ajoute le MD5 trouvé dans la table de hachage
            chunk_index++;//Incrémente l'index des chunks
        }else {
            //Le chunk existe déjà, ainsi pas besoin de le dupliquer
            printf("Chunk %d déjà sauvegardé avec l'index : %d \n",chunk_index,existing_index);
        }
    }
    //Fichier bien dupliqué
    printf("Fichier dédupliqué avec succés. Nombre de chunks unique : %d",chunk_index);
}


// Fonction permettant de charger un fichier dédupliqué en table de chunks
// en remplaçant les références par les données correspondantes

void undeduplicate_file(FILE *file, Chunk **chunks, int *chunk_count) {
    /* @param: file est le nom du fichier dédupliqué présent dans le répertoire de sauvegarde
    *           chunks représente le tableau de chunk qui contiendra les chunks restauré depuis filename
    *           chunk_count est un compteur du nombre de chunk restauré depuis le fichier filename
    */
    unsigned char buffer[CHUNK_SIZE];
    unsigned char md5[MD5_DIGEST_LENGTH];
    int index = 0;

    // Réinitialisation des chunks et du compteur
    *chunks = NULL;
    *chunk_count = 0;

    // Lecture du fichier dédupliqué
    while (fread(md5, 1, MD5_DIGEST_LENGTH, file) == MD5_DIGEST_LENGTH) {
        // Lecture des données du chunk
        if (fread(buffer, 1, CHUNK_SIZE, file) != CHUNK_SIZE) {
            fprintf(stderr, "Erreur: lecture des données du chunk échouée.\n");
            exit(EXIT_FAILURE);
        }

        // Ajout des données au tableau de chunks
        *chunks = realloc(*chunks, sizeof(Chunk) * (*chunk_count + 1));
        if (*chunks == NULL) {
            perror("Erreur d'allocation mémoire pour les chunks");
            exit(EXIT_FAILURE);
        }

        (*chunks)[*chunk_count].data = malloc(CHUNK_SIZE);
        if ((*chunks)[*chunk_count].data == NULL) {
            perror("Erreur d'allocation mémoire pour les données du chunk");
            exit(EXIT_FAILURE);
        }

        memcpy((*chunks)[*chunk_count].data, buffer, CHUNK_SIZE);
        memcpy((*chunks)[*chunk_count].md5, md5, MD5_DIGEST_LENGTH);
        (*chunk_count)++;
    }

    printf("Restauration réussie. Nombre de chunks restaurés : %d\n", *chunk_count);

}
