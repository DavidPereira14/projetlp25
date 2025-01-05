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
#include <limits.h>

// Fonction pour convertir un MD5 en chaîne hexadécimale
char* md5_to_string(unsigned char *md5) {
    static char md5_str[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&md5_str[i * 2], "%02x", md5[i]);
    }
    return md5_str;
}

// Fonction qui vérifie l'éxistance du fichier
int file_exists(const char *filename) {
    // Utilise la fonction access pour vérifier si le fichier existe
    if (access(filename, F_OK) == 0) {
        return 1;  // Le fichier existe
    } else {
        return 0;  // Le fichier n'existe pas
    }
}

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


        char src_path[1024], dest_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", backup_id, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", restore_dir, entry->d_name);

        // copier le fichier spécifique ".backup_log"
        if (strcmp(entry->d_name, ".backup_log") == 0) {
            copy_file(src_path,dest_path);
            continue;
        }

        if (stat(src_path, &src_stat) == -1) {
            perror("Erreur lors de la récupération des informations sur le fichier source");
            continue;
        }

        if (S_ISDIR(src_stat.st_mode)) {
            // Créer le sous-répertoire dans la destination
            mkdir(dest_path,0755);
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
            if (stat(dest_path, &dest_stat) == -1) {
                if (mkdir(dest_path, 0755) == -1) {
                    printf("Erreur lors de la création du dossier destination\n");
                    continue;
                }
            }
            enregistrement(src_path, dest_path);  // Appel récursif
        } else if (S_ISREG(src_stat.st_mode)) {
            if (stat(dest_path, &dest_stat) == -1) {
                copy_file(src_path, dest_path);
                //backup_file(dest_path);
            } else {
                if (src_stat.st_mtime > dest_stat.st_mtime) {
                    copy_file(src_path, dest_path);
                    //backup_file(dest_path);
                }
            }
        }
    }
    closedir(src);

    while ((entry = readdir(dest)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".backup_log") == 0) {
            continue;
        }

        char dest_path[1024], src_path[1024];
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);

        if (stat(dest_path, &dest_stat) == -1) {
            continue;
        }

        if (S_ISDIR(dest_stat.st_mode)) {
            if (stat(src_path, &src_stat) == -1) {
                supprimer_recursivement(dest_path);  // Implémenter cette fonction pour supprimer récursivement les répertoires
            }
        } else {
            if (stat(src_path, &src_stat) == -1) {
                remove(dest_path);
            }
        }
    }
    closedir(dest);

    printf("Sauvegarde terminée et .backup_log mis à jour.\n");

    return 0;
}

// Fonction pour créer une nouvelle sauvegarde complète puis incrémentale
void create_backup(const char *source_dir, const char *backup_dir) {
    if (check_directory(source_dir) == -1) {
        printf("Erreur : vérifier le répertoire source (existence, permission)\n");
        return;
    }
    if (check_directory(backup_dir) == -1) {
        printf("Erreur : vérifier le répertoire backup (existence, permission)\n");
        return;
    }

    // Génération du timestamp pour le nom de la sauvegarde
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    // Création du chemin de sauvegarde
    char backup_path[1024];
    snprintf(backup_path, sizeof(backup_path), "%s/%s", backup_dir, timestamp);

    // Création du fichier .backup_log
    char backup_log_path[1024];
    snprintf(backup_log_path, sizeof(backup_log_path), "%s/.backup_log", backup_path);

    mkdir(backup_path, 0755);

    FILE *log_file = fopen(backup_log_path, "w");
    if (!log_file) {
        perror("Erreur lors de la création du fichier .backup_log");
        return;
    }

    char *last_backup_name = find_last_backup(backup_dir);

    if (last_backup_name) {
        // Sauvegarde incrémentale
        char last_backup_path[1024];
        snprintf(last_backup_path, sizeof(last_backup_path), "%s/%s", backup_dir, last_backup_name);

        copie_backup(last_backup_path, backup_path);
        free(last_backup_name);
    }

    // Lecture de l'ancien backup_log
    log_t logs = read_backup_log(backup_log_path);

    // Lister et sauvegarder les fichiers
    DIR *dir = opendir(source_dir);
    if (!dir) {
        perror("Erreur lors de l'ouverture du répertoire source");
        fclose(log_file);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Chemin complet du fichier source et destination
        char src_file_path[1024], dest_file_path[1024];
        snprintf(src_file_path, sizeof(src_file_path), "%s/%s", source_dir, entry->d_name);
        snprintf(dest_file_path, sizeof(dest_file_path), "%s/%s", backup_path, entry->d_name);

        // Copier le fichier vers le répertoire de sauvegarde
        copy_file(src_file_path, dest_file_path);

        // Récupérer les métadonnées du fichier
        struct stat file_stat;
        if (stat(src_file_path, &file_stat) == -1) {
            perror("Erreur lors de la récupération des métadonnées");
            continue;
        }

        // Création d'un élément de log
        log_element *new_log = malloc(sizeof(log_element));
        if (!new_log) {
            perror("Erreur d'allocation mémoire pour log_element");
            continue;
        }

        new_log->path = strdup(dest_file_path);
        if (!new_log->path) {
            perror("Erreur d'allocation mémoire pour path");
            free(new_log);
            continue;
        }

        new_log->date = malloc(32);
        if (!new_log->date) {
            perror("Erreur d'allocation mémoire pour date");
            free(new_log->path);
            free(new_log);
            continue;
        }
        get_timestamp(new_log->date, 32);

        // Calculer la somme MD5 du fichier avec compute_md5()
        unsigned char file_data[4096];
        FILE *file = fopen(src_file_path, "rb");
        if (!file) {
            perror("Erreur d'ouverture du fichier pour MD5");
            free(new_log->path);
            free(new_log->date);
            free(new_log);
            continue;
        }

        size_t bytes_read = fread(file_data, 1, sizeof(file_data), file);
        fclose(file);

        compute_md5(file_data, bytes_read, new_log->md5);

        // Écrire les logs dans .backup_log
        write_log_element(new_log, log_file);

        // Libération de mémoire
        free(new_log->path);
        free(new_log->date);
        free(new_log);
    }

    closedir(dir);
    fclose(log_file);

    // Mettre à jour le fichier .backup_log
    update_backup_log(backup_log_path, &logs);
}




// Fonction permettant d'enregistrer dans un fichier le tableau de chunks dédupliqué
void write_backup_file(const char *output_filename, Chunk *chunks, int chunk_count) {
    FILE *output_file = fopen(output_filename, "wb");  // Ouvrir le fichier de sortie en mode binaire
    if (!output_file) {
        perror("Erreur d'ouverture du fichier de sauvegarde");
        return;
    }

    // Parcourir tous les chunks et écrire leurs données dans le fichier
    for (int i = 0; i < chunk_count; i++) {
        size_t data_size = strlen((char *)chunks[i].data);  // Taille réelle des données dans ce chunk

        // Écrire le MD5 suivi des données du chunk
        if (fwrite(chunks[i].md5, 1, MD5_DIGEST_LENGTH, output_file) != MD5_DIGEST_LENGTH) {
            perror("Erreur d'écriture du MD5 dans le fichier");
            fclose(output_file);
            return;
        }

        if (fwrite(chunks[i].data, 1, data_size, output_file) != data_size) {
            perror("Erreur d'écriture dans le fichier");
            fclose(output_file);
            return;
        }
    }

    fclose(output_file);  // Fermer le fichier après l'écriture
    printf("Fichier de sauvegarde créé : %s\n", output_filename);
}

// Fonction implémentant la logique pour la sauvegarde d'un fichier
void backup_file(const char *filename) {
    // Vérifier si le fichier source existe
    if (!file_exists(filename)) {
        printf("Le fichier %s n'existe pas.\n", filename);
        return;
    }

    // Ouvrir le fichier source en mode binaire
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erreur d'ouverture du fichier");
        return;
    }

    // Initialiser la table de hachage pour la déduplication et un tableau de chunks pour stocker les données dédupliquées
    Md5Entry hash_table[HASH_TABLE_SIZE] = { 0 };
    Chunk chunks[100];  // Tableau pour stocker les chunks

    // Dédupliquer le fichier en le divisant en chunks et en évitant les doublons
    unsigned char buffer[CHUNK_SIZE];
    size_t bytes_read;
    unsigned char md5[MD5_DIGEST_LENGTH];

    // Appeler la fonction deduplicate_file pour découper le fichier en chunks et éviter les doublons
    deduplicate_file((FILE *) filename, chunks, hash_table);

    // Après la déduplication, enregistrer les chunks dans un fichier de sauvegarde
    write_backup_file("backup_file.bin", chunks, 100);  // Enregistrer les chunks uniques dans un fichier

    // Fermer le fichier source
    fclose(file);

    printf("Sauvegarde du fichier effectuée.\n");
}

// Fonction pour restaurer un fichier à partir des chunks dédupliqués
void write_restored_file(const char *output_filename, Chunk *chunks, int chunk_count) {
    // Ouvrir le fichier de sortie en mode binaire
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("Erreur lors de l'ouverture du fichier de sortie");
        return;
    }

    Md5Entry hash_table[HASH_TABLE_SIZE] = {0};  // Table de hachage pour éviter les doublons

    // Parcourir tous les chunks pour écrire le fichier restauré
    for (int i = 0; i < chunk_count; i++) {
        // Vérifier si le chunk est déjà présent dans la table de hachage
        int index = find_md5(hash_table, chunks[i].md5);

        if (index == -1) {
            // Si le chunk n'est pas trouvé, il est unique, on l'ajoute à la table de hachage
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
            // Si le chunk est déjà dans la table, on peut l'ignorer (il a déjà été écrit)
            printf("Chunk %d déjà écrit (référence trouvée dans la table de hachage).\n", i);
        }
    }

    // Fermer le fichier après avoir écrit tous les chunks
    fclose(output_file);
    printf("Fichier restauré avec succès dans '%s'\n", output_filename);
}

// Fonction de restauration des fichiers à partir d'une sauvegarde
void restore_backup(const char *backup_id, const char *restore_dir) {
    // Vérification du répertoire de destination
    if (check_directory(restore_dir) != 0) {
        return;
    }

    char backup_path[4096];
    snprintf(backup_path, sizeof(backup_path), "%s/%s", restore_dir, backup_id);

    // Vérification si le répertoire de sauvegarde existe
    struct stat st;
    if (stat(backup_path, &st) == -1) {
        perror("Erreur : Le répertoire de sauvegarde n'existe pas ou est inaccessible");
        return;
    }

    // Vérification du fichier de log de la sauvegarde
    char log_file_path[4096];
    snprintf(log_file_path, sizeof(log_file_path), "%s/.backup_log", backup_path);
    log_t logs = read_backup_log(log_file_path);  // Lire le fichier de log
    if (!logs.head) {
        printf("Erreur : Aucun log de sauvegarde trouvé dans %s\n", log_file_path);
        return;
    }

    log_element *current = logs.head;

    // Destination par défaut si non spécifiée
    char default_dest[4096];
    if (!restore_dir) {
        if (!getcwd(default_dest, sizeof(default_dest))) {
            perror("Impossible de déterminer le répertoire courant");
            free(&logs);
            return;
        }
        restore_dir = default_dest;
    }

    // Parcourir les fichiers à restaurer
    while (current != NULL) {
        char src_path[4096], dest_path[4096];
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

    free(&logs);  // Libérer la mémoire allouée pour les logs

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
