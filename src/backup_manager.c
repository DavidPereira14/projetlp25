#include "backup_manager.h"
#include "deduplication.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>

// Fonction pour créer une nouvelle sauvegarde complète puis incrémentale
void create_backup(const char *source_dir, const char *backup_dir) {
    /* @param: source_dir est le chemin vers le répertoire à sauvegarder
    *          backup_dir est le chemin vers le répertoire de sauvegarde
    */
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

// Fonction pour restaurer une sauvegarde
void restore_backup(const char *backup_id, const char *restore_dir) {
    /* @param: backup_id est le chemin vers le répertoire de la sauvegarde que l'on veut restaurer
    *          restore_dir est le répertoire de destination de la restauration
    */
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
