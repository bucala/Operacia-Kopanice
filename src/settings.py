"""Globalne konstanty hry Operacia Kopanice."""

# Okno
SCREEN_W = 1280
SCREEN_H = 720
FPS = 60
TITLE = "Operacia Kopanice - Misia 1: Zelezna hrdelnica"

# Izometricka dlazdica (W_t / H_t = polovicna sirka / vyska)
TILE_W = 64
TILE_H = 32
HALF_W = TILE_W // 2  # W_t
HALF_H = TILE_H // 2  # H_t
LAYER_H = 24          # H_l - pixelovy ofset vyskovej vrstvy

# Terenne typy: (id, nazov, cena pohybu g_m, rychlostny modifikator, vyska, farba, footprints)
TERRAIN = {
    "road":   {"cost": 1.0,  "speed": 1.0,   "height": 0, "color": (110, 104, 92),  "foot": None},
    "floor":  {"cost": 1.0,  "speed": 1.0,   "height": 0, "color": (92, 82, 70),    "foot": None},
    "mud":    {"cost": 2.0,  "speed": 0.5,   "height": 0, "color": (74, 60, 42),    "foot": "mud"},
    "snow":   {"cost": 3.5,  "speed": 0.285, "height": 0, "color": (205, 210, 222), "foot": "snow"},
    "grass":  {"cost": 1.4,  "speed": 0.75,  "height": 0, "color": (66, 84, 54),    "foot": None},
    "wall":   {"cost": float("inf"), "speed": 0.0, "height": 2, "color": (58, 54, 50), "foot": None},
    "rail":   {"cost": 1.2,  "speed": 0.9,   "height": 0, "color": (84, 80, 78),    "foot": None},
}

# Detekcia / videnie
VISION_RANGE = 8.0          # dlaznic
PRIMARY_CONE = 15.0         # stupne (cerveny kuzel) - okamzity Alert
PERIPHERAL_CONE = 45.0      # stupne (zlty kuzel)
ALERT_RATE = 40.0           # +%/s v perifernom kuzeli (stojaci hrac)
ALERT_DECAY = 25.0          # %/s pokles podozrenia
SUSPICIOUS_TIMEOUT = 10.0   # s bez podnetu -> Patrol
ALERT_LOST_TIMEOUT = 20.0   # s bez kontaktu -> Suspicious

# Zvuk / noise (zjednoduseny model utlmu)
NOISE_SHOT = 20.0           # dlaznic dosah vystrelu
NOISE_STEP = 1.5            # dosah krokov (mimo spevnenej cesty)

# Boj
PLAYER_HP = 100
GUARD_SHOOT_RANGE = 7.0
GUARD_SHOOT_DMG = 22
GUARD_SHOOT_CD = 0.8        # s medzi vystrelmi

# Footprints (stopy)
FOOTPRINT_MUD_TTL = 15.0
FOOTPRINT_SNOW_TTL = 9999.0  # trvale v ramci misie

# Farby UI
COL_BG = (16, 20, 18)
COL_PLAYER = (70, 150, 90)
COL_PLAYER_SEL = (140, 230, 160)
COL_SPY = (60, 58, 64)              # tmavy kozeny plast
COL_SPY_DISGUISED = (96, 104, 84)   # feldgrau dostojnicka uniforma
COL_SNIPER = (120, 108, 84)         # zimny plast / kozusinova ciapka
COL_THIEF = (86, 96, 112)           # lahka tmava vystroj
FALSE_ORDER_RANGE = 7.0             # dosah falosneho rozkazu (dlaznic)

# Odstrelovac
SNIPE_RANGE = 14.0                  # dlaznic
SNIPE_AMMO = 5                      # limitovany pocet nabojov
SNIPE_NOISE = 26.0                  # fatalny zvukovy profil vystrelu
COL_GUARD = (170, 70, 60)
COL_GUARD_ALERT = (240, 80, 60)
COL_BODY = (90, 50, 50)
COL_TNT = (200, 160, 60)
COL_PILLAR = (120, 120, 130)
COL_EXIT = (80, 160, 220)
COL_TEXT = (230, 228, 220)
COL_PANEL = (24, 30, 26)

CONE_PRIMARY = (220, 70, 60, 70)
CONE_PERIPH = (220, 200, 70, 36)
