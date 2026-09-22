# Arduino Loop Control (`ArduinoLoopCtrl`)

Ce projet contient le firmware Arduino permettant de piloter un moteur pas à pas via un driver **DRV8825**, principalement destiné au réglage fin d'antennes (ex. antenne boucle magnétique / *magnetic loop antenna*) via une liaison série (USB / RS485).

---

## 📋 Table des matières

1. [Présentation](#-présentation)
2. [Fonctionnalités](#-fonctionnalités)
3. [Câblage et Brochage (Pinout)](#-câblage-et-brochage-pinout)
4. [Gestion du Microstepping](#-gestion-du-microstepping)
5. [Protocole de Communication Série](#-protocole-de-communication-série)
6. [Analyse Détaillée et Particularités du Code](#-analyse-détaillée-et-particularités-du-code)
7. [Structure du Code](#-structure-du-code)
8. [Installation et Utilisation](#-installation-et-utilisation)

---

## 🚀 Présentation

`ArduinoLoopCtrl` est un croquis Arduino (*sketch*) conçu pour contrôler le positionnement d'une antenne en agissant sur un moteur pas à pas relié à un condensateur variable ou un mécanisme d'accord. Les commandes sont transmises via la liaison série sous forme de chaînes ASCII préfixées par le caractère `$`.

Chaque contrôleur écoute le bus série et compare le paramètre d'antenne transmis à sa propre adresse configurée dans la variable `ctrl_ant` (définie par défaut à `0`).

---

## ✨ Fonctionnalités

- **Contrôle d'un driver DRV8825** : Pilotage complet des broches `DIR`, `STEP`, `MS1`, `MS2` et `ENABLE`.
- **Microstepping en temps réel** : Choix entre pas entier, 1/2 pas, 1/4 pas ou 1/8 pas.
- **Économie d'énergie** : Possibilité de couper l'alimentation du moteur entre deux mouvements via `$SOF`.
- **Réception Série Asynchrone** : Traitement non-bloquant des caractères reçus, filtrage du symbole de début `$` et tolérance aux fins de ligne (`\r`, `\n`, `;`).

---

## 🔌 Câblage et Brochage (Pinout)

L'attribution des broches numériques de l'Arduino vers le module DRV8825 est la suivante :

| Broche Arduino | Signal DRV8825 | Description |
| :--- | :--- | :--- |
| **Pin 3** | `DIR` | Sens de rotation (`LOW` = Sens horaire / CW, `HIGH` = Sens anti-horaire / CCW) |
| **Pin 4** | `STEP` | Génération de l'impulsion de pas |
| **Pin 5** | `MS2` | Broche de configuration microstepping MS2 |
| **Pin 6** | `MS1` | Broche de configuration microstepping MS1 |
| **Pin 7** | `ENABLE` | Activation du moteur (`LOW` = Moteur alimenté, `HIGH` = Moteur désactivé) |

---

## ⚙️ Gestion du Microstepping

La résolution des pas (`res`) est spécifiée lors des commandes de préparation au déplacement (`$SINC` ou `$SDEC`).

Le code inverse la valeur transmise en effectuant `res = 3 - res`, puis applique les niveaux logiques sur `MS1` et `MS2` :

| Valeur `res` | `3 - res` | `MS2` (Pin 5) | `MS1` (Pin 6) | Résolution effective |
| :---: | :---: | :---: | :---: | :--- |
| **0** | `3` (`0b11`) | `HIGH` | `HIGH` | **1/8 Micropas** (Précision maximale) |
| **1** | `2` (`0b10`) | `HIGH` | `LOW` | **1/4 Micropas** |
| **2** | `1` (`0b01`) | `LOW` | `HIGH` | **1/2 Micropas** (Demi-pas) |
| **3** | `0` (`0b00`) | `LOW` | `LOW` | **Pas entier** (Full step) |

---

## 📡 Protocole de Communication Série

### Paramètres de la liaison série
- **Vitesse (Baudrate)** : `38400 bauds`
- **Préfixe de début de commande** : `$`
- **Fin de commande** : Caractère de fin de ligne `\r`, `\n` ou `;`

### Format des commandes ASCII

Les commandes sont analysées par la fonction `rs485_parse_incoming()`.

> ⚠️ **Note importante sur la syntaxe des arguments** :
> Le parseur utilise des décalages d'index fixes sur la chaîne reçue après le symbole `$` (par exemple, la commande se trouve au début et les arguments sont lus via `strtol` à des offsets fixes dans le buffer). Le format compact direct est donc préconisé (ex: `$SINC00`).

| Commande | Syntaxe conseillée | Arguments | Description |
| :--- | :--- | :--- | :--- |
| `$SINIT` | `$SINIT<ant>` | `<ant>` (ex: `0`) | Re-initialise les broches d'E/S du contrôleur et désactive le moteur (`drv8825_Init()`). |
| `$SON` | `$SON<ant>` | `<ant>` (ex: `0`) | Active le driver et alimente le moteur (`drv8825_PwrOn()`). |
| `$SOF` | `$SOF<ant>` | `<ant>` (ex: `0`) | Désactive le driver pour couper l'alimentation moteur (`drv8825_PwrOff()`). |
| `$SINC` | `$SINC<ant><res>` | `<ant>` (0-2)<br>`<res>` (0-3) | Arme le déplacement **sens horaire** (CW) avec la résolution de micropas `res`. |
| `$SDEC` | `$SDEC<ant><res>` | `<ant>` (0-2)<br>`<res>` (0-3) | Arme le déplacement **sens anti-horaire** (CCW) avec la résolution `res`. |
| `$SMOV` | `$SMOV<ant>` | `<ant>` (ex: `0`) | Exécute l'impulsion de pas préparée (`drv8825_Move()`). |
| `$SANT` | `$SANT<ant>` | `<ant>` (0-2) | *(Doc seulement)* Mentionné dans l'en-tête mais non implémenté dans le code. |

#### Exemple de séquence typique d'utilisation :
1. `$SINIT0` : Initialise le contrôleur d'antenne 0.
2. `$SON0` : Met sous tension le moteur de l'antenne 0.
3. `$SINC00` : Configure le déplacement en sens horaire à l'adresse 0 avec 1/8 de micropas (`res = 0`).
4. `$SMOV0` : Effectue le pas.
5. `$SOF0` : Coupe l'alimentation du moteur pour éviter toute surchauffe ou interférence radio.

---

## 🔍 Analyse Détaillée et Particularités du Code

Lors d'une ré-analyse approfondie du code source `ArduinoLoopCtrl.ino`, plusieurs caractéristiques spécifiques ont été identifiées :

1. **Extraction des arguments par offset fixe** :
   Dans `rs485_parse_incoming()`, la lecture des arguments `ant` et `res` s'effectue directement à des positions prédéfinies dans le tableau `incoming_command_string` :
   - Pour `$sinc` : `ant` à l'offset `+4`, `res` à l'offset `+5`.
   - Pour `$sdec` : `ant` à l'offset `+4`, `res` à l'offset `+5`.
   - Pour `$smov` : `ant` à l'offset `+4`.
   - Pour `$son` / `$sof` : `ant` à l'offset `+3`.
   - Pour `$sinit` : `ant` à l'offset `+5`.

2. **Commande `$SANT` non implémentée** :
   Le tableau récapitulatif figurant dans les commentaires d'en-tête mentionne la commande `$SANT <ant>`, mais celle-ci n'est pas gérée dans `rs485_parse_incoming()`.

3. **Génération du pas (`drv8825_Move`)** :
   - `drv8825_Incr()` et `drv8825_Decr()` arment la broche `STEP` au niveau haut (`HIGH`).
   - `drv8825_Move()` passe la broche `STEP` au niveau bas (`LOW`), attend `50 ms` via `delay(50)`, puis la repasse au niveau haut (`HIGH`).

4. **Mode "Silencieux" (sans retour série)** :
   Toutes les lignes `Serial.print` ou `Serial.println` renvoyant un accusé de réception (`$m0st...`) sont actuellement commentées dans le code. Le firmware fonctionne en mode unilatéral sans émettre de réponse sur le port série.

---

## 🛠️ Structure du Code

- **`setup()`** : Appelle `drv8825_Init()` et initialise le port série à `38400` bauds.
- **`loop()`** : Exécute en continu `rs485_read_and_parse()`.
- **`rs485_read_and_parse()`** : Lit les octets entrants du buffer série, attend le préfixe `$`, puis accumule les caractères jusqu'à rencontrer `\r`, `\n` ou `;`.
- **`rs485_parse_incoming()`** : Identifie la commande demandée et vérifie si le numéro d'antenne correspond à `ctrl_ant` avant d'exécuter la fonction DRV8825 associée.
- **Fonctions DRV8825** :
  - `drv8825_Init()` : Configure les 5 broches en `OUTPUT` et met le moteur hors tension.
  - `drv8825_PwrOn()` / `drv8825_PwrOff()` : Pilote la broche `ENABLE`.
  - `drv8825_Incr()` / `drv8825_Decr()` : Choisit la direction (`DIR`), configure `MS1` / `MS2` et prépare `STEP` à `HIGH`.
  - `drv8825_Move()` : Génère le creux d'impulsion de 50 ms pour accomplir le pas.

---

## 📥 Installation et Utilisation

1. **Prérequis** :
   - [Arduino IDE](https://www.arduino.cc/en/software) ou [PlatformIO](https://platformio.org/).
   - Carte Arduino (Nano, Uno, Pro Mini, etc.) reliée à un driver DRV8825.

2. **Téléversement** :
   - Ouvrir `ArduinoLoopCtrl.ino`.
   - Sélectionner le type de carte et le port série adéquat.
   - Téléverser le programme.

3. **Test via un terminal série** :
   - Configurer le terminal sur **38400 bauds**.
   - Envoyer les commandes ASCII (ex. `$SON0`, `$SINC03`, `$SMOV0`, `$SOF0`).
