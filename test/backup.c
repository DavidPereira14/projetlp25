#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    MD5(data, len, md5_out);
}

// Fonction permettant de chercher un MD5 dans la table de hachage
int find_md5(Md5Entry *hash_table, unsigned char *md5) {
    /* @param: hash_table est le tableau de hachage qui contient les MD5 et l'index des chunks unique
    *           md5 est le md5 du chunk dont on veut déterminer l'unicité
    *  @return: retourne l'index s'il trouve le md5 dans le tableau et -1 sinon
    */
    int resultat = -1;
    int i = 0;
    while (resultat == -1 && i < HASH_TABLE_SIZE){
        if (strcmp(hash_table[i].md5,md5) == 0){
            resultat = hash_table[i].index;
        }
        i++;
    }
    return resultat;
}

// Ajouter un MD5 dans la table de hachage
void add_md5(Md5Entry *hash_table, unsigned char *md5, int index) {
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        hash_table[index].md5[i] = md5[i];
    }
    hash_table[index].index = index;
}

// Fonction pour convertir un fichier non dédupliqué en tableau de chunks
void deduplicate_file(FILE *file, Chunk *chunks, Md5Entry *hash_table){
    /* @param:  file est le fichier qui sera dédupliqué
    *           chunks est le tableau de chunks initialisés qui contiendra les chunks issu du fichier
    *           hash_table est le tableau de hachage qui contient les MD5 et l'index des chunks unique
    */
}


// Fonction permettant de charger un fichier dédupliqué en table de chunks
// en remplaçant les références par les données correspondantes
void undeduplicate_file(FILE *file, Chunk **chunks, int *chunk_count) {
    /* @param: file est le nom du fichier dédupliqué présent dans le répertoire de sauvegarde
    *           chunks représente le tableau de chunk qui contiendra les chunks restauré depuis filename
    *           chunk_count est un compteur du nombre de chunk restauré depuis le fichier filename
    */

}
