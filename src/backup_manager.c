#include "backup_manager.h"
#include "deduplication.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>

// Fonction pour vérifier si un répertoire existe et qu'on a les permissions
int check_directory(const char *path) {
    struct stat path_stat;

    // Vérifier si le chemin existe
    if (stat(path, &path_stat) == -1) {
        perror("Erreur : Le chemin n'existe pas");
        return -1; // Erreur
    }

    // Vérifier si c'est un répertoire
    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "Erreur : Le chemin n'est pas un répertoire\n");
        return -1; // Erreur
    }

    // Vérifier les permissions (lecture, écriture, exécution)
    if (access(path, R_OK) == -1) {
        perror("Erreur : Pas de permission de lecture sur le répertoire");
        return -1; // Erreur
    }

    if (access(path, W_OK) == -1) {
        perror("Erreur : Pas de permission d'écriture sur le répertoire");
        return -1; // Erreur
    }

    if (access(path, X_OK) == -1) {
        perror("Erreur : Pas de permission d'exécution sur le répertoire");
        return -1; // Erreur
    }

    printf("Le répertoire '%s' existe et toutes les permissions sont disponibles\n", path);
    return 0; // Succès
}

// Fonction pour obtenir l'horodatage formaté
void get_timestamp(char *buffer, size_t size) {
    struct timespec ts;
    struct tm *tm_info;

    clock_gettime(CLOCK_REALTIME, &ts);
    tm_info = localtime(&ts.tv_sec);

    snprintf(buffer, size, "%04d-%02d-%02d-%02d:%02d:%02d.%03ld",
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             ts.tv_nsec / 1000000);
}

// Fonction pour trouver la dernière sauvegarde (par ordre alphabétique des noms)
char *find_last_backup(const char *dest_dir) {
    DIR *dir;
    struct dirent *entry;
    char *last_backup = NULL;

    dir = opendir(dest_dir);
    if (!dir) {
        perror("Erreur lors de l'ouverture du répertoire destination");
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (!last_backup || strcmp(entry->d_name, last_backup) > 0) {
            free(last_backup);
            last_backup = strdup(entry->d_name);
        }
    }

    closedir(dir);
    return last_backup;
}

int are_files_different(const char *file1, const char *file2) {
    FILE *f1 = fopen(file1, "rb");
    FILE *f2 = fopen(file2, "rb");

    // Cas où le fichier n'existe que dans le dossier 1
    if (f1 && !f2) {
        fclose(f1);
        return 1; // Fichier seulement dans le dossier 1
    }

    // Cas où le fichier n'existe que dans le dossier 2
    if (!f1 && f2) {
        fclose(f2);
        return -1; // Fichier seulement dans le dossier 2
    }

    // Cas où les deux fichiers existent
    if (f1 && f2) {
        return 2; // Fichier dans les 2 dossiers
    }
}

void copier_fichier(const char *source, const char *destination) {
    FILE *src = fopen(source, "rb");  // Ouvre le fichier source en mode binaire
    if (src == NULL) {
        perror("Erreur d'ouverture du fichier source");
        return;
    }

    FILE *dest = fopen(destination, "wb");  // Ouvre le fichier destination en mode binaire
    if (dest == NULL) {
        perror("Erreur d'ouverture du fichier destination");
        fclose(src);
        return;
    }

    char buffer[4096];  // Buffer pour lire et écrire des données
    size_t bytes_read;

    // Lire et copier le contenu du fichier source dans le fichier destination
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes_read, dest);
    }

    printf("Le fichier '%s' a été copié vers '%s'.\n", source, destination);

    fclose(src);  // Ferme le fichier source
    fclose(dest);  // Ferme le fichier destination
}



int enregistrement(const char *src_dir, const char *dest_dir){
    DIR *src = opendir(src_dir);
    DIR *dest = opendir(dest_dir);
    if (!dest) {
        printf("Erreur d'ouverture du répertoire destination");
        return -1;
    }
    if (!src){
        printf("Erreur : lors de l'ouverture du répertoire source");
        return -1;
    }
    struct dirent *entry;
    struct stat src_stat;
    struct stat dest_stat;
    while ((entry = readdir(src)) != NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char src_path[1024], dest_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        if (stat(src_path, &src_stat) == -1) {
            printf("Erreur lors de la récupération des informations source");
            continue;
        }
        if (S_ISDIR(src_stat.st_mode)) {
            // Gestion des dossiers
            if (stat(dest_path, &dest_stat) == -1) {
                // Le dossier n'existe pas dans la destination, on le crée
                if (mkdir(dest_path, 0755) == -1) {
                    printf("Erreur lors de la création du dossier destination");
                    continue;
                }
            }
            // Synchroniser récursivement
            enregistrement(src_path, dest_path);
        } else if (S_ISREG(src_stat.st_mode)) {
            // Gestion des fichiers
            int diff = are_files_different(src_path, dest_path);
            switch (diff) {
                case -1:
                    remove(dest_path);
                    break;
                case 1:
                    copier_fichier(src_path,dest_path);
                    break;
                case 2:
                    //appeler fonction toto
                    break;
            }
        }

    }

}



// Fonction pour créer une nouvelle sauvegarde complète puis incrémentale
void create_backup(const char *source_dir, const char *backup_dir) {
    DIR *src;
    struct dirent *entry;
    struct stat src_stat, dest_stat;
    if (check_directory(source_dir)==-1){
        printf("Erreur : vérifier le repertoire source (existance,permission)");
        return;
    }
    if (check_directory(backup_dir)==-1){
        printf("Erreur : vérifier le repertoire backup (existance,permission)");
        return;
    }
    // Obtenir l'horodatage pour le répertoire de sauvegarde
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    char backup_path[1024];
    snprintf(backup_path, sizeof(backup_path), "%s/%s", backup_dir, timestamp);
    char *last_backup_name = find_last_backup(backup_dir);
    if (last_backup_name){
        char last_backup_path[1024];
        snprintf(last_backup_path, sizeof(last_backup_path), "%s/%s", backup_dir, last_backup_name);
        printf("Copie complète depuis : %s\n", last_backup_path);
        mkdir(backup_path, 0755);
        restore_backup(last_backup_path, backup_path);
        free(last_backup_name);
    }else{
        // Pas de sauvegarde existante, créer un répertoire vide
        mkdir(backup_path, 0755);
        fopen(strcat(backup_path,"/.backup_log"),"w");
        // copie repertoire
    }
    //
    closedir(src);
    printf("Sauvegarde incrémentale terminée : %s\n", backup_path);
}

// Fonction permettant d'enregistrer dans fichier le tableau de chunk dédupliqué
void write_backup_file(const char *output_filename, Chunk *chunks, int chunk_count) {
    /*
    */
}


// Fonction implémentant la logique pour la sauvegarde d'un fichier
void backup_file(const char *filename) {
    /*
    */
}


// Fonction permettant la restauration du fichier backup via le tableau de chunk
void write_restored_file(const char *output_filename, Chunk *chunks, int chunk_count) {
    /*
    */
}

void restore_backup(const char *backup_id, const char *restore_dir) {
    DIR *src;
    struct dirent *entry;
    struct stat src_stat;

    src = opendir(backup_id);
    if (!src) {
        perror("Erreur lors de l'ouverture du répertoire source");
        return;
    }

    // Parcourir les fichiers et répertoires
    while ((entry = readdir(src)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[1024], dest_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", backup_id, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", restore_dir, entry->d_name);

        if (stat(src_path, &src_stat) == -1) {
            perror("Erreur lors de la récupération des informations sur le fichier source");
            continue;
        }

        if (S_ISDIR(src_stat.st_mode)) {
            // Créer le sous-répertoire dans la destination
            mkdir(dest_path, 0755);
            restore_backup(src_path, dest_path);
        } else {
            // Créer un lien dur vers le fichier source
            if (link(src_path, dest_path) == -1) {
                perror("Erreur lors de la création d'un lien dur");
            }
        }
    }

    closedir(src);
}


// Fonction permettant de lister les différentes sauvegardes présentes dans la destination
void list_backups(const char *backup_dir){
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    // Ouvrir le répertoire spécifié
    dir = opendir(backup_dir);
    if (dir == NULL) {
        perror("Erreur lors de l'ouverture du répertoire");
        return;
    }

    printf("Liste des sauvegardes dans %s:\n", backup_dir);

    // Parcourir les fichiers et dossiers
    while ((entry = readdir(dir)) != NULL) {
        // Ignorer les entrées spéciales "." et ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", backup_dir, entry->d_name);

        // Récupérer les informations sur le fichier
        if (stat(full_path, &file_stat) == -1) {
            perror("Erreur lors de la récupération des informations sur le fichier");
            continue;
        }

        // Vérifier si c'est un répertoire
        if (S_ISDIR(file_stat.st_mode)) {
            printf("- %s\n", entry->d_name);
        }
    }

    closedir(dir);
}
