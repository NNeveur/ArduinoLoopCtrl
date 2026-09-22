# Arduino Loop Control (`ArduinoLoopCtrl`)

Ce projet contient le firmware Arduino permettant de piloter un moteur pas à pas via un driver **SparkFun Big easy driver Allegro A4983 or A4988 stepper driver chip**, principalement destiné au réglage fin d'antennes (ex. antenne boucle magnétique) via une liaison série (RS485).

---

## 📋 Table des matières

1. [Présentation](#-présentation)
2. [Fonctionnalités](#-fonctionnalités)
3. [Câblage et Brochage (Pinout)](#-câblage-et-brochage-pinout)
4. [Gestion du Microstepping](#-gestion-du-microstepping)
5. [Protocole de Communication Série](#-protocole-de-communication-série)
6. [Structure du Code](#-structure-du-code)
7. [Installation et Utilisation](#-installation-et-utilisation)

---

## 🚀 Présentation

`ArduinoLoopCtrl` est un croquis Arduino (*sketch*) conçu pour contrôler le positionnement d'une antenne en agissant sur un moteur pas à pas. Les commandes sont transmises via le port série  RS485 sous forme de chaînes ASCII préfixées par le caractère `$`.

Le contrôleur vérifie l'identifiant de l'antenne spécifié dans chaque commande (`ctrl_ant`) afin de ne traiter que les instructions qui lui sont destinées.

---

## ✨ Fonctionnalités

- **Contrôle d'un driver Big easy** : Gestion des broches `DIR`, `STEP`, `MS1`, `MS2`, `MS3` et `ENABLE`.
- **Résolution configurable en temps réel** : Support du pas entier jusqu'au 1/16 de pas (microstepping).
- **Économie d'énergie** : Activation/désactivation de l'alimentation des bobines du moteur via commande série.
- **Protocole Série Robuste** : Filtrage des caractères parasites, analyse syntaxique tolérante aux fins de ligne (`\r`, `\n`, `;`).

---

## 🔌 Câblage et Brochage (Pinout)

Le code définit l'attribution des broches numériques de l'Arduino vers le module DRV8825 comme suit :

| Broche Arduino | Signal DRV8825 | Description |
| :--- | :--- | :--- |
| **Pin 3** | `DIR` | Sens de rotation (LOW = Sens horaire / CW, HIGH = Sens anti-horaire / CCW) |
| **Pin 4** | `STEP` | Impulsion d'exécution d'un pas |
| **Pin 5** | `ENABLE` | Activation du moteur (LOW = Moteur actif, HIGH = Moteur hors tension) |
| **Pin 6** | `MS3` | Sélection du mode microstepping (Bit 2)|
| **Pin 7** | `MS2` | Sélection du mode microstepping (Bit 1) |
| **Pin 8** | `MS1` | Sélection du mode microstepping (Bit 0) |

*Note : La variable `ctrl_ant` (définie à `0` dans le code) correspond à l'adresse de l'antenne assignée à ce contrôleur.*

---

## ⚙️ Gestion du Microstepping

La résolution des pas (`res`) est configurable lors de la préparation des mouvements d'incrémentation ou de décrémentation (`$SINC` et `$SDEC`).

Le DRV8825 décode la résolution inversée (`3 - res`) selon la table suivante :

| Valeur `res` | Conversion (`4 - res`) | `MS2` (Pin 6) | `MS1` (Pin 7) | `MS0` (Pin 8) | Résolution du pas |
| :---: | :---: | :---: | :---: | :--- |
| **0** | `4` (`0b111`) | HIGH | HIGH | HIGH | **1/16 Micropas** (Microstepping maximal) |
| **1** | `3` (`0b011`) | LOW | HIGH | HIGH | **1/8 Micropas** |
| **2** | `2` (`0b010`) | LOW | HIGH | LOW | **1/4 Micropas** |
| **3** | `1` (`0b001`) | LOW | LOW | HIGH | **1/2 Micropas** (Demi-pas) |
| **4** | `0` (`0b000`) | LOW | LOW | LOW | **Pas entier** (Full step) |

---

## 📡 Protocole de Communication Série

### Paramètres de la liaison série
- **Vitesse (Baudrate)** : `38400 bauds`
- **Délimiteur de début** : `$`
- **Fin de commande** : Renvoi à la ligne (`\r`, `\n`) ou point-virgule `;`

### Format des commandes

Chaque commande reçue doit commencer par le caractère d'attention `$`, suivi du code de la commande et des arguments séparés par des espaces.

| Commande | Arguments | Description |
| :--- | :--- | :--- |
| `$SINIT<ant>` | `<ant>` (0-2) | Réinitialise les sorties du contrôleur pour l'antenne ciblée et désactive le moteur. |
| `$SON<ant>` | `<ant>` (0-2) | Active l'alimentation du moteur pas à pas. |
| `$SOF<ant>` | `<ant>` (0-2) | Coupe l'alimentation du moteur (mise en veille / économie d'énergie). |
| `$SINC<ant><res>` | `<ant>` (0-2)<br>`<res>` (0-4) | Arme un déplacement dans le **sens horaire** pour l'antenne ciblée avec la résolution `res`. |
| `$SDEC<ant><res>` | `<ant>` (0-2)<br>`<res>` (0-4) | Arme un déplacement dans le **sens anti-horaire** pour l'antenne ciblée avec la résolution `res`. |
| `$SMOV<ant>` | `<ant>` (0-2) | Exécute le déplacement d'un pas (génère l'impulsion `STEP`). |
| `$SANT<ant>` | `<ant>` (0-2) | Sélection / commutation de l'antenne active sur le bus RS485. |

#### Exemple de séquence de commande :
1. `$SINIT0` : Initialise le contrôleur pour l'antenne 0.
2. `$SON0` : Met sous tension le moteur.
3. `$SINC00` : Prépare un déplacement horaire en 1/16 de micropas (`res = 0`).
4. `$SMOV0` : Exécute le pas.
5. `$SOF0` : Desactive le moteur pour éviter l'échauffement ou la consommation inutile.

---

## 🛠️ Structure du Code

- **`setup()`** : Initialise le driver DRV8825 et démarre la communication série à `38400` bauds.
- **`loop()`** : Exécute en boucle la fonction `rs485_read_and_parse()`.
- **`rs485_read_and_parse()`** : Lit les octets arrivant sur le port série, filtre jusqu'à trouver le préfixe `$`, puis accumule le message jusqu'au délimiteur de fin de ligne.
- **`rs485_parse_incoming()`** : Analyse la commande ASCII et appelle la fonction moteur correspondante si l'identifiant d'antenne correspond à `ctrl_ant`.
- **Fonctions stepctrl** :
  - `stepctrl_Init()` : Configure les broches en sortie et coupe le moteur.
  - `stepctrl_PwrOn()` / `stepctrl_PwrOff()` : Active / désactive le signal `ENABLE`.
  - `stepctrl_Incr(res)` / `stepctrl_Decr(res)` : Définit la direction, ajuste le microstepping et prépare le pas.
  - `stepctrl_Move()` : Envoie l'impulsion `LOW -> delay(50ms) -> HIGH` sur la broche `STEP`.

---

## 📥 Installation et Utilisation

1. **Prérequis** :
   - [Arduino IDE](https://www.arduino.cc/en/software) (version 1.8.x ou 2.x).
   - Carte Arduino (ex. Nano, Uno) reliée au driver Big easy.

2. **Téléversement** :
   - Ouvrir le fichier `ArduinoLoopCtrl.ino` dans l'Arduino IDE.
   - Sélectionner le type de carte et le port COM approprié.
   - Téléverser le croquis.

3. **Test via le Moniteur Série** :
   - Ouvrir le Moniteur Série (*Serial Monitor*).
   - Régler le débit sur **38400 bauds**.
   - Envoyer des commandes telles que `$SON0;`, `$SINC03;`, `$SMOV0;`, `$SOF0;`.
