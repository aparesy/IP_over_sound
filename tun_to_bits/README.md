# tun_to_bits — Lecture de paquets TUN → encapsulation de trame → conversion trame en bit

Parallèlement à `wav_modulator`, implémentant la première moitié de la chaîne de transmission. Chaque étape est un fichier distinct (y compris le fichier d'en-tête) et le programme principal utilise **le même fichier exécutable + différents paramètres** pour prendre en charge une exécution et une vérification indépendantes étape par étape.

---

# Utilisation détaillée (anglais)

## Présentation

Le programme de programmation **tun_to_bits** permet d'exécuter, sans précédent, quatre étapes suivantes : **créer le TUN** → **lire unpaquet depuis le TUN** → **encapsuler le paquet en trame** → **convertir la trame en flux de bits**. La différence entre l'argument et la chaque étape.

##Compilation

```bash
cd tun_to_bits
make
```

L’exécutable généré s’appelle **tun_to_bits**.

## Exécution pas à pas (ordre réel) et vérification

### Étape 1 : Créer le TUN

**Commande:**

```bash
sudo ./tun_to_bits --create tun0
```

**Vérification du succès :**

- Le programme affiche : **`TUN créé avec succès : tun0 (fd=<num>)`**
- Le programme se termine sans message d'erreur.
- Optionnel : `ip link show tun0` affiche l'interface tun0.

**Encas d’échec:** message **`TUN create failed`** — exécuter avec **sudo**.

---

### Étape 2 : Lire un paquet depuis le TUN

**Prérequis (une seule fois):** Donner une adresse à tun0 et l'activer:

```bash
sudo ip addr ajouter 10.0.0.1/24 dev tun0
lien sudo ip configuré tun0
```

(Adresse déjà attribuée » : on peut l’ignorer et ne lancer que la seconde si besoin.)

**Commande:**

```bash
sudo ./tun_to_bits --read tun0
```

Le programme **attend** qu’un paquet soit envoyé vers le TUN. Dans **un autre terminal**, envoyez un Paquet contre sous-réseau 10.0.0.0/24, par exemple :

```bash
ping -c 1 10.0.0.2
```

**Vérification du succès :**

- Le terminal qui exécute `--read` affiche : **`Lire le paquet avec succès : N octets -> output/ip.bin`** (N > 0).
- Le fichier **output/ip.bin** existe et fait environ N octets (`ls -la output/ip.bin`).

**Encas d’échec :** Le programme reste bloqué (aucun paquet n’arrive sur le TUN) ou affiche **`Read packet failed or no data`**. Vérifiez que l’autre terminal envoie bien du trafic vers 10.0.0.x et que la route passe par tun0.

---

### Étape 3 : Encapsuler le paquet en trame (IP → trame)

**Commande:**

```bash
./tun_to_bits --encapsulate
```

(Pas besoin de root. Le fichier **output/ip.bin** doit exister, produit par l’étape 2.)

**Vérification du succès :**

- Affichage : **`Encapsuler avec succès : IP N octets -> Frame M octets -> output/frame.bin`** (N et M positifs, avec M = 4 + N + 2).
- Le fichier **output/frame.bin** existe et fait M octets.

**En cas d'échec :** message **`Cannot read output/ip.bin (run --read first)`** ou **`encapsulate failed`**. Exécuter d’abord l’étape 2 avec succès.

---

### Étape 4 : Convertir la trame en bits

**Commande:**

```bash
./tun_to_bits --to-bits
```

(Pas besoin de root. Le fichier **output/frame.bin** doit exister, produit par l’étape 3.)

**Vérification du succès :**

- Affichage : **`Convertir la trame en bits avec succès : Trame M octets -> K bits -> sortie/bits.bin`** (K = M × 8).
- Le fichier **output/bits.bin** existe et fait (K+7)/8 octets.

**Encas d’échec :** message **`Cannot read output/frame.bin (run --encapsulate first)`**. Exécuter d’abord l’étape 3 avec succès.

---

### Enchaînement des étapes et résumé

| Ordre | Commande | Sucès si |
|-------|----------|------------|
| 1 | `sudo ./tun_to_bits --create tun0` | Affichage **`TUN créé avec succès`**, le programme se termine normalement. |
| 2 | (Dans l'autre terminal : `ping -c 1 10.0.0.2`) puis `sudo ./tun_to_bits --read tun0` | Affichage **`Le paquet a été lu avec succès : N octets -> output/ip.bin`**, fichier **output/ip.bin** présent. |
| 3 | `./tun_to_bits --encapsulate` | Affichage **`Encapsulate success`**, fichier **output/frame.bin** présent. |
| 4 | `./tun_to_bits --to-bits` | Affichage **`Convert frame to bits with success`**, fichier **output/bits.bin** présent. |

Une fois le fichier **output/bits.bin** obtenu, on peut l’utiliser avec le module de modulation (wav_modulator) pour créer un fichier WAV :

```bash
cd ../wav_modulator
./bits_to_wav ../tun_to_bits/output/bits.bin output/from_tun.wav
```

(Clip chinois)

---

| Fichier | Fonction | |------|------|

| **tun_create.c/h** | Crée un périphérique TUN (encapsule tun_open) |

| **packet_read.c/h** | Lit un paquet IP depuis un périphérique TUN et le stocke dans un tampon (encapsule tun_read) |

| **encapsulate.c/h** | Encapsule le paquet IP dans une trame (encapsule protocol_encapsulate) |

| **frame_to_bits.c/h** | Convertit une image en flux binaire |

| **tun_to_bits.c** | Programme principal : Exécute des opérations pas à pas ou un processus complet en fonction des paramètres |

L'implémentation sous-jacente réutilise toujours `tun_dev`, `protocol` et `utils` du projet.

## Compilation

```bash
cd tun_to_bits
make
```
Génère le fichier exécutable `tun_to_bits`.

## Utilisation (Même fichier exécutable, paramètres différents)

**Exécution pas à pas, vérification indépendante à chaque étape :**

| Paramètre | Fonction | Entrée | Sortie | Vérification |

|------|------|------|------|

| `--create [tun_name]` | Crée uniquement le TUN | - | - | Afficher « TUN créée avec succès » |

| `--read [nom_tun]` | Lire un seul paquet depuis la TUN | - | output/ip.bin | Afficher « Paquet lu avec succès : N octets » |

| `--encapsulate` | Encapsuler uniquement dans des trames | output/ip.bin | output/frame.bin | Afficher « Encapsulation réussie » |

| `--to-bits` | Convertir les trames en bits uniquement | output/frame.bin | output/bits.bin | Afficher « Conversion de bits réussie » |

**Ordre recommandé (tester d'abord les tests 3 et 4, puis les tests 1 et 2) :**