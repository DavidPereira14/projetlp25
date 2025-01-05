#include "backup_manager.h"
#include "deduplication.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>


// Fonction pour récupérer le dernier répertoire de sauvegarde
char *get_last_backup(const char *backup_dir) {
    DIR *dir = opendir(backup_dir);
    if (!dir) {
        perror("Erreur lors de l'ouverture du répertoire de sauvegarde");
        return NULL;
    }

    struct dirent *entry;
    struct stat st;
    time_t latest_time = 0;
    char *latest_folder = NULL;

    // Parcours des fichiers et dossiers dans le répertoire de sauvegarde
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", backup_dir, entry->d_name);

            if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (st.st_mtime > latest_time) {
                    latest_time = st.st_mtime;
                    free(latest_folder);
                    latest_folder = strdup(entry->d_name);
                }
            }
        }
    }
    closedir(dir);
    return latest_folder;
}

void create_backup(const char *source_dir, const char *backup_dir) {
    // Ouverture du répertoire source
    DIR *dir = opendir(source_dir);
    if (!dir) {
        perror("Impossible d'ouvrir le répertoire source");
        return;
    }

    // Vérification que le répertoire de sauvegarde existe
    struct stat st;
    if (stat(backup_dir, &st) == -1) {
        perror("Erreur d'accès au répertoire de sauvegarde");
        closedir(dir);
        return;
    }

    // Obtenir l'horodatage pour le nom du dossier de sauvegarde
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char backup_folder[256];
    strftime(backup_folder, sizeof(backup_folder), "%Y-%m-%d-%H:%M:%S", tm_info);

    // Création du répertoire de sauvegarde
    char backup_path[512];
    snprintf(backup_path, sizeof(backup_path), "%s/%s", backup_dir, backup_folder);
    if (mkdir(backup_path, 0755) == -1) {
        perror("Erreur lors de la création du répertoire de sauvegarde");
        closedir(dir);
        return;
    }

    // Chemin vers le fichier .backup_log
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/.backup_log", backup_dir);

    // Récupération du dernier répertoire de sauvegarde
    char *last_backup_folder = get_last_backup(backup_dir);
    if (last_backup_folder) {
        printf("Dernier répertoire de sauvegarde : %s\n", last_backup_folder);
        char last_backup_path[512];
        snprintf(last_backup_path, sizeof(last_backup_path), "%s/%s", backup_dir, last_backup_folder);

        // Lecture du fichier .backup_log pour obtenir les fichiers sauvegardés
        log_t logs = read_backup_log(log_path);
        log_element *current = logs.head;

        // Création de liens durs pour les fichiers existants
        while (current) {
            char src_file[512], dest_file[512];
            snprintf(src_file, sizeof(src_file), "%s/%s", last_backup_path, current->path);
            snprintf(dest_file, sizeof(dest_file), "%s/%s", backup_path, current->path);

            // Création du lien dur
            if (link(src_file, dest_file) == -1) {
                perror("Erreur lors de la création d'un lien dur");
            }

            current = current->next;
        }
    } else {
        // Si aucun fichier .backup_log n'existe, on en crée un nouveau
        FILE *log_file = fopen(log_path, "w");
        if (!log_file) {
            perror("Erreur lors de la création du fichier .backup_log");
            closedir(dir);
            return;
        }
        fclose(log_file);
    }

    // Nettoyage
    free(last_backup_folder);
    closedir(dir);
}
