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
#include <sys/syslimits.h>

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

// Fonction pour supprimer un fichier ou un dossier récursivement
int supprimer_recursivement(const char *chemin) {
    struct stat chemin_stat;

    // Vérifier si le chemin existe et obtenir ses informations
    if (stat(chemin, &chemin_stat) != 0) {
        perror("Erreur lors de l'obtention des informations sur le chemin");
        return -1;
    }

    // Si c'est un fichier, on le supprime
    if (S_ISREG(chemin_stat.st_mode) || S_ISLNK(chemin_stat.st_mode)) {
        if (remove(chemin) == 0) {
            printf("Fichier supprimé : %s\n", chemin);
            return 0;
        } else {
            perror("Erreur lors de la suppression du fichier");
            return -1;
        }
    }

    // Si c'est un répertoire, on traite son contenu récursivement
    if (S_ISDIR(chemin_stat.st_mode)) {
        DIR *dir = opendir(chemin);
        if (!dir) {
            perror("Erreur lors de l'ouverture du répertoire");
            return -1;
        }

        struct dirent *entry;
        char sous_chemin[1024];

        // Parcourir les entrées du répertoire
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue; // Ignorer les répertoires spéciaux
            }

            snprintf(sous_chemin, sizeof(sous_chemin), "%s/%s", chemin, entry->d_name);

            // Suppression récursive du contenu
            if (supprimer_recursivement(sous_chemin) != 0) {
                closedir(dir);
                return -1;
            }
        }
        closedir(dir);

        // Supprimer le répertoire une fois vide
        if (rmdir(chemin) == 0) {
            printf("Répertoire supprimé : %s\n", chemin);
            return 0;
        } else {
            perror("Erreur lors de la suppression du répertoire");
            return -1;
        }
    }

    fprintf(stderr, "Chemin non supporté : %s\n", chemin);
    return -1;
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


void copie_backup(const char *backup_id, const char *restore_dir) {
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
        // Ignorer le fichier spécifique ".backup_log"
        if (strcmp(entry->d_name, ".backup_log") == 0) {
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
            mkdir(dest_path);
            copie_backup(src_path, dest_path);
        } else {
            // Créer un lien dur vers le fichier source
            if (link(src_path, dest_path) == -1) {
                perror("Erreur lors de la création d'un lien dur");
            }
        }
    }
    closedir(src);
}

int enregistrement(const char *src_dir, const char *dest_dir) {
    DIR *src = opendir(src_dir);
    DIR *dest = opendir(dest_dir);
    if (!dest) {
        printf("Erreur d'ouverture du répertoire destination\n");
        return -1;
    }
    if (!src) {
        printf("Erreur lors de l'ouverture du répertoire source\n");
        return -1;
    }

    struct dirent *entry;
    struct stat src_stat;
    struct stat dest_stat;
    // Créer le chemin complet du fichier .backup_log
    char log_path[1024];
    snprintf(log_path, sizeof(log_path), "%s/.backup_log", dest_dir);

    // Ouvrir le fichier .backup_log en mode lecture/écriture
    FILE *log_file = fopen(log_path, "r+"); // Utilise "a+" pour ajouter au fichier sans écraser son contenu
    if (log_file == NULL) {
        printf("Erreur lors de l'ouverture du fichier .backup_log\n");
        return -1; // Retourner une erreur si l'ouverture échoue
    }
    while ((entry = readdir(src)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[1024], dest_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        if (stat(src_path, &src_stat) == -1) {
            printf("Erreur lors de la récupération des informations source\n");
            continue;
        }

        if (S_ISDIR(src_stat.st_mode)) {
            // Gestion des dossiers
            if (stat(dest_path, &dest_stat) == -1) {
                // Le dossier n'existe pas dans la destination, on le crée
                if (mkdir(dest_path) == -1) {
                    printf("Erreur lors de la création du dossier destination\n");
                    continue;
                }
            }
            // Synchronisation récursive pour les sous-dossiers
            enregistrement(src_path, dest_path);
        } else if (S_ISREG(src_stat.st_mode)) {
            // Gestion des fichiers
            if (stat(dest_path, &dest_stat) == -1) {
                // Le fichier n'existe pas dans la destination, on le copie
                copy_file(src_path, dest_path);
                backup_file(dest_path);
                //appeler la fonction write_log_element(log_element *elt, FILE *logfile)
            } else {

                if (src_stat.st_mtime > dest_stat.st_mtime) {
                    // Fichier source plus récent ou contenu différent
                    copy_file(src_path, dest_path);
                    backup_file(dest_path);
                } else {
                    // Les fichiers sont identiques
                    // Rien à faire
                }
            }
        }
    }
    closedir(src);
    // Vérification des fichiers dans la destination qui ne sont plus présents dans la source
    while ((entry = readdir(dest)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (strcmp(entry->d_name, ".backup_log") == 0 ) {
            continue;
        }
        char dest_path[1024], src_path[1024];
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);

        if (stat(dest_path, &dest_stat) == -1) {
            printf("Erreur lors de la récupération des informations destination\n");
            continue;
        }

        if (S_ISDIR(dest_stat.st_mode)) {
            // Gestion des dossiers
            if (stat(src_path, &src_stat) == -1) {
                // Le dossier n'existe pas dans la destination, on le supprime
                supprimer_recursivement(dest_path);
            }


            if (stat(src_path, &src_stat) == -1) {
                // Si le fichier n'existe plus dans la source, on le supprime
                remove(dest_path);
            }
        }
    }
    closedir(dest);
    fclose(log_file);

    // Copie du fichier .backup_log dans le répertoire de sauvegarde
    char backup_log_dest[1024];
    snprintf(backup_log_dest, sizeof(backup_log_dest), "%s/.backup_log", dest_dir);
    copy_file(log_path,
              backup_log_dest); // Fonction pour copier le fichier de log dans le répertoire de la sauvegarde

    printf("Sauvegarde terminée et .backup_log mis à jour.\n");

    return 0;

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
        mkdir(backup_path);
        copie_backup(last_backup_path, backup_path);
        free(last_backup_name);
    }else{
        // Pas de sauvegarde existante, créer un répertoire vide
        mkdir(backup_path);
        fopen(strcat(backup_path,"/.backup_log"),"w");
    }
    log_t logs=read_backup_log(backup_path);
    // Sauvegarder les logs dans le fichier
    update_backup_log(backup_path, &logs);

}

// Fonction permettant d'enregistrer dans fichier le tableau de chunk dédupliqué
void write_backup_file(const char *output_filename, Chunk *chunks, int chunk_count) {
    /*
    */
    printf("\nRAS\n");
}

// Fonction implémentant la logique pour la sauvegarde d'un fichier
void backup_file(const char *filename) {
    /*
    */
    printf("\nne fais rien sur le fichier %s\n",filename);
}


// Fonction permettant la restauration du fichier backup via le tableau de chunk
// Fonction pour restaurer un fichier à partir des chunks dédupliqués
void write_restored_file(const char *output_filename, Chunk *chunks, int chunk_count) {
    // Ouvrir le fichier de sortie en mode binaire
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("Erreur lors de l'ouverture du fichier de sortie");
        return;
    }

    Md5Entry hash_table[HASH_TABLE_SIZE] = {0}; // Table de hachage pour éviter les doublons

    // Parcourir tous les chunks
    for (int i = 0; i < chunk_count; i++) {
        // Vérifier si le chunk est déjà présent dans la table de hachage (pour éviter de réécrire les mêmes données)
        int index = find_md5(hash_table, chunks[i].md5);

        if (index == -1) {
            // Si le chunk n'est pas trouvé dans la table de hachage, il est unique, on l'ajoute à la table
            add_md5(hash_table, chunks[i].md5, i);
            // Écrire les données du chunk dans le fichier de sortie
            size_t chunk_size = CHUNK_SIZE;  // Taille fixe du chunk
            size_t bytes_written = fwrite(chunks[i].data, 1, chunk_size, output_file);
            if (bytes_written != chunk_size) {
                fprintf(stderr, "Erreur lors de l'écriture du chunk %d\n", i);
                fclose(output_file);
                return;
            }
        } else {
            // Si le chunk est déjà dans la table, cela signifie que nous avons déjà écrit ce chunk
            // Nous pouvons juste référencer le chunk sans avoir à écrire à nouveau les mêmes données
            printf("Chunk %d déjà écrit (référence trouvée dans la table de hachage).\n", i);
        }
    }

    // Fermer le fichier une fois la restauration terminée
    fclose(output_file);
    printf("Fichier restauré avec succès dans '%s'\n", output_filename);
}
// Fonction de restauration des fichiers à partir d'une sauvegarde
void restore_backup(const char *backup_id, const char *restore_dir) {
    // Vérification du répertoire de destination
    if (check_directory(restore_dir) != 0) {
        return;
    }

    char backup_path[PATH_MAX];
    snprintf(backup_path, sizeof(backup_path), "%s/%s", restore_dir, backup_id);

    // Vérification si le répertoire de sauvegarde existe
    struct stat st;
    if (stat(backup_path, &st) == -1) {
        perror("Erreur : Le répertoire de sauvegarde n'existe pas ou est inaccessible");
        return;
    }

    // Vérification du fichier de log de la sauvegarde
    char log_file_path[PATH_MAX];
    snprintf(log_file_path, sizeof(log_file_path), "%s/.backup_log", backup_path);
    log_t logs = read_backup_log(log_file_path);  // Lire le fichier de log
    if (!logs.head) {
        printf("Erreur : Aucun log de sauvegarde trouvé dans %s\n", log_file_path);
        return;
    }

    log_element *current = logs.head;

    // Destination par défaut si non spécifiée
    char default_dest[PATH_MAX];
    if (!restore_dir) {
        if (!getcwd(default_dest, sizeof(default_dest))) {
            perror("Impossible de déterminer le répertoire courant");
            free_log(&logs);
            return;
        }
        restore_dir = default_dest;
    }

    // Parcourir les fichiers à restaurer
    while (current != NULL) {
        char src_path[PATH_MAX], dest_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", backup_path, current->path);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", restore_dir, current->path);

        struct stat src_stat, dest_stat;

        // Vérifier si le fichier existe dans la sauvegarde
        if (stat(src_path, &src_stat) == -1) {
            perror("Erreur lors de la lecture du fichier source");
            current = current->next;
            continue;
        }

        // Vérifier si le fichier existe déjà dans la destination
        int file_exists = (stat(dest_path, &dest_stat) != -1);

        // Si le fichier existe déjà, vérifier les conditions pour le remplacer
        if (file_exists) {
            // Vérifier si la date de modification est postérieure
            if (src_stat.st_mtime > dest_stat.st_mtime) {
                copy_file(src_path, dest_path);
            }
            // Vérifier si la taille diffère
            else if (src_stat.st_size != dest_stat.st_size) {
                copy_file(src_path, dest_path);
            }
        } else {
            // Si le fichier n'existe pas, le copier
            copy_file(src_path, dest_path);
        }

        current = current->next;
    }

    free_log(&logs);  // Libérer la mémoire allouée pour les logs

    printf("Restauration terminée dans le répertoire '%s'\n", restore_dir);
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
