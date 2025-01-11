void restore_backup(const char *backup_id, const char *restore_dir) {
    printf("Restore_backup\n\n");

    char backup_log_path[MAX_PATH];
    if (snprintf(backup_log_path, sizeof(backup_log_path), "%s/.backup_log", backup_id) >= sizeof(backup_log_path)) {
        fprintf(stderr, "Erreur : chemin de log trop long.\n");
        return;
    }

    log_t logs = read_backup_log(backup_log_path);
    log_element *current = logs.head;

    if (current == NULL) {
        printf("Aucun élément dans le log de sauvegarde.\n");
        return;
    }

    while (current != NULL) {
        char backup_file_path[MAX_PATH];

        // Si le chemin de `current->path` est déjà absolu ou contient `backup_id`, ne pas préfixer
        if (strncmp(current->path, backup_id, strlen(backup_id)) == 0) {
            strncpy(backup_file_path, current->path, sizeof(backup_file_path));
        } else {
            if (snprintf(backup_file_path, sizeof(backup_file_path), "%s/%s", backup_id, current->path) >= sizeof(backup_file_path)) {
                fprintf(stderr, "Erreur : chemin de fichier trop long pour '%s'.\n", current->path);
                current = current->next;
                continue;
            }
        }

        printf("Tentative d'ouverture du fichier : %s\n", backup_file_path);
        FILE *backup_file = fopen(backup_file_path, "rb");
        if (!backup_file) {
            fprintf(stderr, "Erreur lors de l'ouverture du fichier de sauvegarde '%s': %s\n", backup_file_path, strerror(errno));
            current = current->next;
            continue;
        }

        // Récupération et traitement des chunks
        Chunk *chunks = NULL;
        int chunk_count = 0;
        undeduplicate_file(backup_file, &chunks, &chunk_count);
        fclose(backup_file);

        if (chunk_count == 0) {
            current = current->next;
            continue;
        }

        // Création du répertoire cible
        char restored_file_path[MAX_PATH];
        snprintf(restored_file_path, sizeof(restored_file_path), "%s/%s", restore_dir, current->path);

        char dir_path[MAX_PATH];
        strncpy(dir_path, restored_file_path, sizeof(dir_path));
        char *last_slash = strrchr(dir_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            if (mkdir(dir_path, 0755) != 0 && errno != EEXIST) {
                perror("Erreur lors de la création du répertoire");
                free(chunks);
                current = current->next;
                continue;
            }
        }

        // Écriture du fichier restauré
        if (write_restored_files(restored_file_path, chunks, chunk_count) != 0) {
            fprintf(stderr, "Échec de la restauration de '%s'.\n", current->path);
        }

        free(chunks);
        current = current->next;
    }

    update_backup_log(backup_log_path, &logs);
}
