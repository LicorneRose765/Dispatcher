

void *guichet_behavior(void *arg) {
    /**
     * 1. Créer un guichet
     * 2. Attendre une demande
     * 3. Traiter la demande
     * 4. Retour à l'étape 2
    */
    unsigned int id = gettid();
    task_t type_demande = TASK1; // TODO : random
}