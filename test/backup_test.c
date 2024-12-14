#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/md5.h>
#include <sys/stat.h> // Pour mkdir

#define CHUNK_SIZE 1024
#define HASH_TABLE_SIZE 100

// Structure pour un chunk de données
typedef struct {
    unsigned char *data;
    unsigned char md5[MD5_DIGEST_LENGTH];
} Chunk;

// Structure pour la table de hachage MD5
typedef struct {
    unsigned char md5[MD5_DIGEST_LENGTH];
    int index;
} Md5Entry;

// Fonction permettant de vérifier l'existence d'un fichier
int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

// Fonction pour vérifier si un répertoire existe, et le créer si nécessaire
int directory_exists(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        // Le répertoire n'existe pas, essayons de le créer
        if (mkdir(path, 0777) == 0) {
            printf("Répertoire %s créé avec succès.\n", path);
            return 1;
        } else {
            perror("Erreur lors de la création du répertoire");
            return 0;
        }
    }
    return S_ISDIR(info.st_mode);  // Si c'est un répertoire, retourne 1
}

// Fonction de hachage MD5 pour l'indexation dans la table de hachage
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
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (memcmp(hash_table[i].md5, md5, MD5_DIGEST_LENGTH) == 0) {
            return hash_table[i].index;
        }
    }
    return -1;
}

// Ajouter un MD5 dans la table de hachage
void add_md5(Md5Entry *hash_table, unsigned char *md5, int index) {
    unsigned int position = hash_md5(md5);
    while (hash_table[position].index != -1) {
        position = (position + 1) % HASH_TABLE_SIZE;
    }
    memcpy(hash_table[position].md5, md5, MD5_DIGEST_LENGTH);
    hash_table[position].index = index;
}

// Fonction pour dédupliquer un fichier
void deduplicate_file(FILE *file, Chunk *chunks, Md5Entry *hash_table) {
    unsigned char buffer[CHUNK_SIZE];
    int chunk_index = 0;
    size_t bytes_read;
    unsigned char md5[MD5_DIGEST_LENGTH];

    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        compute_md5(buffer, bytes_read, md5);
        int existing_index = find_md5(hash_table, md5);

        if (existing_index == -1) {
            chunks[chunk_index].data = malloc(bytes_read);
            if (!chunks[chunk_index].data) {
                perror("Erreur d'allocation mémoire");
                exit(EXIT_FAILURE);
            }
            memcpy(chunks[chunk_index].data, buffer, bytes_read);
            memcpy(chunks[chunk_index].md5, md5, MD5_DIGEST_LENGTH);
            add_md5(hash_table, md5, chunk_index);
            chunk_index++;
        } else {
            printf("Chunk déjà existant à l'index %d\n", existing_index);
        }
    }

    printf("Fichier dédupliqué avec succès. Nombre de chunks uniques : %d\n", chunk_index);
}

// Fonction pour sauvegarder un fichier en utilisant des chunks
void backup_file(const char *filename, const char *backup_folder) {
    if (!file_exists(filename)) {
        printf("Le fichier %s n'existe pas.\n", filename);
        return;
    }

    // Vérifier si le répertoire de sauvegarde existe, sinon le créer
    if (!directory_exists(backup_folder)) {
        return; // Si le répertoire n'existe pas et ne peut pas être créé, on arrête l'exécution
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erreur d'ouverture du fichier");
        return;
    }

    Md5Entry hash_table[HASH_TABLE_SIZE] = { 0 };
    Chunk chunks[100];  // Tableau pour stocker les chunks

    deduplicate_file(file, chunks, hash_table);

    fclose(file);
    printf("Sauvegarde du fichier effectuée.\n");
}

// Fonction principale pour tester les fonctionnalités
int main() {
    const char *filename = "testfile.txt";  // Remplacez par le nom d'un fichier valide sur votre système
    const char *backup_folder = "./backup";  // Répertoire de sauvegarde

    backup_file(filename, backup_folder);

    return 0;
}