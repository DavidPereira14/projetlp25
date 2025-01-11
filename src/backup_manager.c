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
#include <errno.h>

#define MAX_PATH 1024  // Définir MAX_PATH comme 1024 caractère




int creer_repertoire(const char *chemin) {
    if (mkdir(chemin, 0755) == 0) {
        printf("Répertoire '%s' créé avec succès.\n", chemin);
        return 0;
    } else {
        if (errno == EEXIST) {
            printf("Le répertoire '%s' existe déjà.\n", chemin);
        } else {
            perror("Erreur lors de la création du répertoire");
        }
        return -1;
    }
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

// Fonction récursive pour calculer la taille d'un dossier
long long calculer_taille_dossier(const char *chemin) {
    DIR *dossier = opendir(chemin);
    if (!dossier) {
        perror("Erreur d'ouverture du dossier");
        return -1;
    }

    struct dirent *entree;
    struct stat info;
    long long taille_totale = 0;

    while ((entree = readdir(dossier)) != NULL) {
        // Ignorer les entrées spéciales "." et ".."
        if (strcmp(entree->d_name, ".") == 0 || strcmp(entree->d_name, "..") == 0) {
            continue;
        }

        // Construire le chemin complet de l'entrée
        char chemin_complet[1024];
        snprintf(chemin_complet, sizeof(chemin_complet), "%s/%s", chemin, entree->d_name);

        // Obtenir les informations sur l'entrée
        if (stat(chemin_complet, &info) == -1) {
            perror("Erreur d'accès aux informations du fichier");
            continue;
        }

        if (S_ISREG(info.st_mode)) {
            // Si c'est un fichier, ajouter sa taille
            taille_totale += info.st_size;
        } else if (S_ISDIR(info.st_mode)) {
            // Si c'est un dossier, appeler récursivement la fonction
            long long taille_sous_dossier = calculer_taille_dossier(chemin_complet);
            if (taille_sous_dossier != -1) {
                taille_totale += taille_sous_dossier;
            }
        }
    }

    closedir(dossier);
    return taille_totale;
}

void appel_write(char *file_path, FILE *logfile) {
    printf("Appel_write\n");
    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) {
        perror("Erreur lors de la récupération des métadonnées36");
        return;
    }

    log_element *new_log = malloc(sizeof(log_element));
    if (!new_log) {
        perror("Erreur d'allocation mémoire pour log_element");
        return;
    }

    new_log->path = strdup(file_path);
    new_log->date = malloc(32);
    if (!new_log->path || !new_log->date) {
        perror("Erreur d'allocation mémoire pour path ou date");
        free(new_log->path);
        free(new_log);
        return;
    }
    get_timestamp(new_log->date, 32);

    // Calcul du MD5
    unsigned char file_data[4096];
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Erreur d'ouverture du fichier pour MD5");
        free(new_log->path);
        free(new_log->date);
        free(new_log);
        return;
    }
    size_t bytes_read = fread(file_data, 1, sizeof(file_data), file);
    fclose(file);

    compute_md5(file_data, bytes_read, new_log->md5);

    // Vérifie si logfile est ouvert avant d'écrire
    if (!logfile) {
        fprintf(stderr, "Erreur : fichier de log non ouvert.\n");
        free(new_log->path);
        free(new_log->date);
        free(new_log);
        return;
    }

    write_log_element(new_log, logfile);

    free(new_log->path);
    free(new_log->date);
    free(new_log);
}

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

int enregistrement(const char *src_dir, const char *dest_dir,FILE *logfile) {
    printf("Enregistrement \n");
    DIR *src = opendir(src_dir);
    if (!src) {
        perror("Erreur lors de l'ouverture du répertoire source");
        return -1;
    }

    DIR *dest = opendir(dest_dir);
    if (!dest) {
        perror("Erreur lors de l'ouverture du répertoire destination");
        closedir(src);
        return -1;
    }

    struct dirent *entry;
    struct stat src_stat, dest_stat;

    // Parcourir les fichiers dans le répertoire source
    while ((entry = readdir(src)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[1024], dest_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        if (stat(src_path, &src_stat) == -1) {
            perror("Erreur lors de la récupération des informations source");
            continue;
        }

        if (S_ISDIR(src_stat.st_mode)) {
            if (stat(dest_path, &dest_stat) == -1) {
                if (mkdir(dest_path, 0755) == -1) {
                    perror("Erreur lors de la création du dossier destination");
                    continue;
                }
            }
            enregistrement(src_path, dest_path,logfile);  // Appel récursif
        } else if (S_ISREG(src_stat.st_mode)) {
            if (stat(dest_path, &dest_stat) == -1 || src_stat.st_mtime > dest_stat.st_mtime) {
                copy_file(src_path, dest_path);
                backup_file(dest_path);
                appel_write(dest_path, logfile);
            }
        }
    }

    // Vérifier les fichiers dans le répertoire destination
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

        if (stat(src_path, &src_stat) == -1) {
            supprimer_recursivement(dest_path);
        }
    }

    // Fermeture des répertoires et du fichier de log
    closedir(src);
    closedir(dest);
    printf("Sauvegarde terminée et fichier .backup_log mis à jour.\n");
    return 0;
}

// Fonction pour créer une nouvelle sauvegarde complète puis incrémentale
void create_backup(const char *source_dir, const char *backup_dir) {
    printf("create_backup");
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

    // Appel de la fonction enregistrement pour faire le backup incrémental
    enregistrement(source_dir, backup_path,log_file);

    fclose(log_file);

    // Mettre à jour le fichier .backup_log
    update_backup_log(backup_log_path, &logs);
}




// Fonction permettant d'enregistrer dans un fichier le tableau de chunks dédupliqué
void write_backup_file(const char *output_filename, Chunk *chunks, int chunk_count) {
    printf("Write_backup_file");
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

// Fonction permettant de sauvegarder un fichier en appliquant la déduplication
void backup_file(const char *filename) {
    printf("Backup_file\n");

    // Ouvrir le fichier à sauvegarder en mode binaire
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    // Initialiser la table de hachage pour les MD5 des chunks
    Md5Entry hash_table[HASH_TABLE_SIZE] = {0};  // Table de hachage pour éviter les doublons

    // Allouer un tableau dynamique pour les chunks (vérifier le nombre d'éléments)
    Chunk *chunks = malloc(sizeof(Chunk) * CHUNK_SIZE);
    if (chunks == NULL) {
        perror("Erreur d'allocation mémoire pour les chunks");
        fclose(file);
        return;
    }

    int chunk_count = 0;

    // Dédupliquer le fichier et le découper en chunks
    deduplicate_file(file, chunks, &chunk_count);

    // Si aucun chunk n'a été trouvé, afficher une erreur et arrêter la sauvegarde
    if (chunk_count == 0) {
        fprintf(stderr, "Aucun chunk à sauvegarder pour '%s'.\n", filename);
        free(chunks);
        fclose(file);
        return;
    }

    // Fermer le fichier après la lecture
    fclose(file);

    // Appeler la fonction pour écrire les chunks dans un fichier de sauvegarde
    char backup_filename[MAX_PATH];
    snprintf(backup_filename, sizeof(backup_filename), "backup/%s_backup.dat", filename);
    write_backup_file(backup_filename, chunks, chunk_count);

    // Libérer la mémoire des chunks
    for (int i = 0; i < chunk_count; i++) {
        free(chunks[i].data);  // Assurez-vous de libérer correctement la mémoire de chaque chunk
    }
    free(chunks);

    printf("Sauvegarde de '%s' terminée avec succès.\n", filename);
}

// Fonction pour restaurer un fichier à partir des chunks dédupliqués
int write_restored_files(const char *output_filename, Chunk *chunks, int chunk_count) {
    printf("Write_restored_files \n\n");
    // Ouvrir le fichier de sortie en mode binaire
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("Erreur lors de l'ouverture du fichier de sortie");
        return -1;
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
            size_t bytes_written = fwrite(chunks[i].data, 1, CHUNK_SIZE, output_file);
        } else {
            // Si le chunk est déjà dans la table, il a déjà été écrit
            printf("Chunk %d déjà écrit (référence trouvée dans la table de hachage).\n", i);
        }
    }

    // Fermer le fichier après avoir écrit tous les chunks
    fclose(output_file);
    printf("Fichier restauré avec succès dans '%s'\n", output_filename);
    return 0;
}

void restore_backup(const char *backup_id, const char *restore_dir) {
    /*
     * @param: backup_id est le chemin vers le fichier de sauvegarde .backup_log
     *         restore_dir est le répertoire où les fichiers seront restaurés
     */
    printf("Restore_backup \n\n");

    // Lire les logs depuis le fichier
    log_t logs = read_backup_log(backup_id);

    log_element *current = logs.head;  // Accéder au premier élément de la liste de logs

    if (current == NULL) {
        printf("Current est vide\n");
        return;  // Sortir si la liste de logs est vide
    }

    while (current != NULL) {
        // Construire le chemin complet du fichier restauré
        char restored_file_path[MAX_PATH];
        snprintf(restored_file_path, sizeof(restored_file_path), "%s/%s", restore_dir, current->path);

        // Vérifier si le fichier existe déjà et s'il doit être restauré
        struct stat file_stat;
        if (stat(restored_file_path, &file_stat) == 0) {
            // Comparer la date de dernière modification (timestamp) avec la sauvegarde
            struct tm tm_info;
            strptime(current->date, "%Y-%m-%d %H:%M:%S", &tm_info);
            time_t backup_time = mktime(&tm_info);

            if (file_stat.st_mtime >= backup_time) {
                printf("Fichier '%s' déjà à jour, restauration ignorée.\n", current->path);
                current = current->next;  // Passer à l'élément suivant de la liste
                continue;
            }
        }

        // Ouvrir le fichier de sauvegarde correspondant
        FILE *backup_file = fopen(current->path, "rb");
        if (!backup_file) {
            perror("Erreur lors de l'ouverture du fichier de sauvegarde");
            current = current->next;  // Passer à l'élément suivant de la liste
            continue;
        }

        // Charger les chunks à partir du fichier sauvegardé
        Chunk *chunks = NULL;
        int chunk_count = 0;
        undeduplicate_file(backup_file, &chunks, &chunk_count);
        fclose(backup_file);

        if (chunk_count == 0) {
            fprintf(stderr, "Aucun chunk à restaurer pour '%s'.\n", current->path);
            current = current->next;  // Passer à l'élément suivant de la liste
            continue;
        }

        // Créer le répertoire de destination s'il n'existe pas
        char dir_path[MAX_PATH];
        strncpy(dir_path, restored_file_path, sizeof(dir_path));
        char *last_slash = strrchr(dir_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            if (mkdir(dir_path, 0755) != 0 && errno != EEXIST) {
                perror("Erreur lors de la création du répertoire de restauration");
                free(chunks);
                current = current->next;  // Passer à l'élément suivant de la liste
                continue;
            }
        }

        // Restaurer le fichier depuis les chunks
        int write = write_restored_files(restored_file_path, chunks, chunk_count);
        if (write == 0) {
            printf("Fichier '%s' restauré avec succès.\n", current->path);
        } else {
            fprintf(stderr, "Échec de la restauration de '%s'.\n", current->path);
        }

        free(chunks);

        // Passer à l'élément suivant dans la liste chaînée
        current = current->next;
    }

    // Mettre à jour le fichier .backup_log après la restauration
    update_backup_log(backup_id, &logs);
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
            long long taille = calculer_taille_dossier(full_path);
            if (taille == -1) {
                fprintf(stderr, "Erreur lors du calcul de la taille du dossier.\n");
                continue;
            }
            printf("- %s taille de l'enregistrement : %lld octets\n", entry->d_name,taille);
        }
    }

    closedir(dir);
}
