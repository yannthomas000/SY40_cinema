# SY40_cinema
Sujet : Gestion du Cinéma
Simuler un système complet de gestion d'un cinéma comprenant plusieurs salles de projection.
Le système doit gérer la réservation et l'achat de billets, la gestion des salles, l'échange de
billets et la gestion des restrictions d'accès à certains films. Les clients peuvent interagir avec
des hôtesses ou des guichets automatiques pour acheter, réserver ou échanger des billets.
Idées de fonctionnalités pouvant être implémentées :
1. Gestion des Places de Cinéma :
• Chaque salle a un nombre limité de places disponibles, numérotées.
• Le nombre de places disponibles doit être mis à jour en temps réel et accessible à tout
moment, que ce soit via un guichet automatique ou par une hôtesse.
• Les places sont numérotées et les clients peuvent choisir une place spécifique lors de
l'achat (si disponible).
2. Achat de Billets :
• Les clients peuvent acheter des billets via deux moyens :
1. Guichets automatiques : Interface plus rapide, mais moins flexible pour les
échanges ou demandes spéciales.
2. Hôtesses : Interface plus personnalisée, capable de gérer des demandes
spéciales, comme des échanges de billets ou des vérifications d'âge.
• Le système doit garantir que l'achat d'un billet est atomique (les places sont réservées
sans risque de collision entre plusieurs clients).
3. Réservations :
• Les clients peuvent réserver une place à l'avance. Les clients avec réservation sont
prioritaires sur ceux qui achètent sur place.
• Le système doit être capable de gérer les annulations de réservations et remettre les
places annulées dans le pool de places disponibles.
4. Projection Flexible :
• Chaque salle peut changer de film si la demande pour un autre film est plus élevée que
pour le film actuellement programmé, sous certaines conditions :
o Si moins de 20% des places sont vendues pour un film et un autre film a une
demande supérieure, la salle peut modifier sa programmation.
• Cette fonctionnalité nécessite une gestion dynamique des salles et de la programmation
en fonction des demandes clients.
5. Échange de Billets :
• Les clients peuvent échanger leur billet pour un autre film ou une autre séance auprès
d'une hôtesse.
• L'échange doit tenir compte des places disponibles et proposer des alternatives aux
clients si leur demande initiale n'est plus possible (changement de place, autre salle,
autre film).
6. Gestion des Restrictions d'Âge :
• Certains films sont soumis à des restrictions d'âge.
• Lors de l'achat du billet via une hôtesse, le client peut être interrogé pour confirmer son
âge. En cas de doute, l'hôtesse peut interdire l'accès au film et proposer un échange de
billet pour un film approprié.
7. Priorité aux Réservations :
• Les clients ayant réservé leur place à l'avance sont prioritaires sur ceux qui achètent leur
billet à la dernière minute.
• Le système doit gérer cette priorité, en donnant la préférence aux réservations si
plusieurs clients souhaitent acheter la même place.
8. Gestion Multi-Salles et Multi-Films :Le cinéma doit pouvoir gérer plusieurs salles avec des films différents. Chaque salle a
ses propres règles de gestion des places et des films.
• La capacité de chaque salle varie, et certaines salles peuvent être réservées pour des
événements spéciaux.
9. Interface Utilisateur pour le Guichet Automatique et l'Hôtesse :
• Créer deux interfaces :
1. Guichet automatique : Permet à un client d'acheter ou de réserver un billet en
temps réel, de vérifier la disponibilité des places et de choisir sa place.
2. Interface pour l'hôtesse : Permet à l'hôtesse de vendre des billets, d'effectuer
des échanges, et de gérer les restrictions d'accès (âge).
10. Optimisation des Files d'Attente :
• Gérer les files d'attente des clients au guichet automatique et auprès des hôtesses.
• Les clients ayant une réservation doivent passer en priorité dans la file d'attente.
11. Annulation et Remboursement :
• Le système doit permettre aux clients d'annuler une réservation ou un billet acheté,
selon certaines conditions (par exemple, avant un délai de 30 minutes avant la séance).
• En cas d'annulation, le système doit gérer les remboursements et remettre la place
disponible pour d'autres clients.
12. Notification des Clients :
• En cas de changement de film ou de modification de la salle, les clients ayant une
réservation ou ayant acheté un billet doivent être notifiés de la modification.
• Notifications envoyées sous forme d'email ou par affichage sur l'écran du guichet
automatique.
13. Rapport de Ventes et de Statistiques :
• Le système doit générer des rapports statistiques pour le cinéma :
o Nombre de billets vendus par film.
o Taux de remplissage des salles.
o Temps d'attente moyen des clients aux guichets.
o Nombre d'échanges de billets effectués.
• Ces rapports doivent être accessibles par l'administration du cinéma pour des prises de
décision.
14. Vérification Automatique des Capacités :
• Si une salle atteint un certain pourcentage de remplissage (par exemple, 90%), une
alerte doit être déclenchée pour limiter l'accès et ne plus accepter de réservations ou
d'achats de billets pour cette séance.
•
![image](https://github.com/user-attachments/assets/4ea7216b-1feb-4761-b212-2080cbc3b80d)
![image](https://github.com/user-attachments/assets/f6f105bf-171a-43c7-9e7e-8f176b1088c4)


