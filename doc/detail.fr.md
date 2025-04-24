# Détail de ce proxy IZAR

## Raison

Suite à une fuite enterrée j'ai découvert que ma ville ne faisait pas de télérelève régulière des compteurs, mais seulement une relève tous les 6 mois et du coup ne notifiait pas les sur consommation ni les fuites alors que le compteur les détecte.

Je suis équipé d'un compteur avec une bague DIEHL, en fouillant sur internet j'ai vu que les informations de consommation était diffusé presque en clair. (récupérable par exemple avec le projet [wmbusmeters](https://github.com/wmbusmeters/wmbusmeters))

J'ai d'abord essayé de capter les trames avec une carte TI CC1101, il y a une extension pour ESPHOME qui fait cela. Cela marche bien, j'ai réussi à récupérer les trames avec une carte [E07-900M10S](https://www.cdebyte.com/products/E07-900M10S). J'ai même un développement qui marche sur la carte de dev qui possède un STM8  [carte de dev](https://www.cdebyte.com/products/E07-900MBL-01).

Le problème est la distance depuis le compteur. Au delà de 2 mètres cela ne marche plus et mon compteur est à plus de 100 mètres de mon domicile.

Pour les longues distances j'ai déjà utilisé Lorawan, j'ai une gateway TTN chez moi, j'ai donc envisagé de connecter le CC1101 à une carte que j'utilise déjà. Mais en regardant de plus prés, je me suis aperçu que le WMBUS utilisé du FSK ce qui est déjà supporté par les modules radio SX1276 et j'évite donc d'avoir à connecter les deux.

## La carte

La carte est quelque chose que j'avais déjà. Elle est composé d'un arduino pro mini à 8MHz, réglé pour fonctionné à 2MHz afin de fonctionner avec seulement deux batterie AA sans régulateur step up (le 328P peux fonctionner jusqu'a 1.8V si on limite la fréquence), connecté à un SX1276 (RFM95). Du coup la carte fonctionne avec une tension de d'environ 2,4V, en utilisant le mode deepsleep j'ai déjà réussi à faire tenir une carte comme cela plus d'un an avec seulement de batterie AA NiMh pour un capteur de mesure de température.

On peux sûrement utilisé un processeur plus moderne moins gourmand dans une conception nouvelle.

## Fonctionnement

Le programme est basé autour de la librairie que je maintien LMICPP est un portage de LMIC, une librairie Lorawan pour Arduino, en C++.

Il a le fonctionnement suivant :
  - Il vérifie si il y a une action à faire coté LoraWAN, si c'est le cas il l'execute (envoie de trame, attente de réponse, ...).
  - Si la librairie LoraWAN n'a rien de prévu dans les 90 prochaine seconde et qu'aucune trame n'a été envoyé depuis plus de 5 minutes, le programme bascule en mode écoute WMBUS
  - L'écoute se fait jusqu'à la réception d'une trame valide et qui correspond au compteur écouté.
  - Si une trame valide est détecté et décodé correctement, les données de status et de compteur sont envoyé.
  - Sinon l'état de la batterie est envoyé.

## Résultat

Cela fait un mois que le système fonctionne, il manque régulièrement des points de mesures mais un trou ne dépasse jamais plus de 15 min.
La carte est dans une boite étanche enterré en partie à 1 mètre du compteur.