extends Node2D

const TILE_W := 64.0
const TILE_H := 32.0
const MAP_W := 20
const MAP_H := 16
const DARK_RANGE := 2.4
const LIGHT_RANGE := 6.4
const CONE_ANGLE := deg_to_rad(54.0)
const MOVE_SPEED := 2.9
const CRAWL_SPEED := 1.35

var specialists := [
	{"name":"Jack O'Hara", "role":"Zelený baret", "color":Color(0.15,0.58,0.25), "tool":"nôž + návnada", "pos":Vector2i(2,12), "alive":true, "crouch":false},
	{"name":"Sir Francis T. Woolridge", "role":"Odstreľovač", "color":Color(0.62,0.72,0.85), "tool":"3 tiché výstrely", "pos":Vector2i(2,13), "alive":true, "crouch":false, "ammo":3},
	{"name":"James Blackwood", "role":"Mariňák", "color":Color(0.1,0.35,0.8), "tool":"čln + vrhací nôž", "pos":Vector2i(1,13), "alive":true, "crouch":false},
	{"name":"Thomas Hancock", "role":"Ženista", "color":Color(0.95,0.55,0.12), "tool":"dynamit + kliešte", "pos":Vector2i(3,13), "alive":true, "crouch":false},
	{"name":"Sid Perkins", "role":"Vodič", "color":Color(0.8,0.45,0.2), "tool":"vozidlá + guľomet", "pos":Vector2i(1,12), "alive":true, "crouch":false},
	{"name":"René Duchamp", "role":"Špión", "color":Color(0.58,0.34,0.88), "tool":"uniforma + jed", "pos":Vector2i(3,12), "alive":true, "crouch":false, "disguised":false}
]

var terrain := []
var guards := []
var bodies := []
var selected := 0
var targets := []
var alarm := false
var alarm_timer := 0.0
var message := "Vyber špecialistu (1-6), klikni na cieľ a zostaň mimo tmavozeleného kužeľa. F5/F9 rýchlo ulož/načítaj."
var saved_state := {}
var camera_offset := Vector2(560, 90)

func _ready() -> void:
	_build_terrain()
	_build_guards()
	targets.resize(specialists.size())
	for i in specialists.size():
		targets[i] = Vector2(specialists[i].pos)
	queue_redraw()

func _process(delta: float) -> void:
	_update_guards(delta)
	_update_specialists(delta)
	_check_detection(delta)
	queue_redraw()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		var key := event.keycode
		if key >= KEY_1 and key <= KEY_6:
			selected = clampi(key - KEY_1, 0, specialists.size() - 1)
			message = "%s: %s" % [specialists[selected].role, specialists[selected].tool]
		elif key == KEY_C:
			specialists[selected].crouch = not specialists[selected].crouch
			message = "Plazenie: %s" % ("zapnuté" if specialists[selected].crouch else "vypnuté")
		elif key == KEY_K:
			_try_silent_takedown()
		elif key == KEY_H:
			_try_hide_body()
		elif key == KEY_U and selected == 5:
			specialists[selected].disguised = true
			message = "Špión si oblieka ukradnutú uniformu a môže zdržať hliadku."
		elif key == KEY_D and selected == 5:
			_try_distract_guard()
		elif key == KEY_F5:
			_quick_save()
		elif key == KEY_F9:
			_quick_load()
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		var tile := _screen_to_tile(event.position)
		if _in_bounds(tile) and terrain[tile.y][tile.x] != "blocked":
			targets[selected] = Vector2(tile)

func _build_terrain() -> void:
	for y in MAP_H:
		var row := []
		for x in MAP_W:
			var t := "grass"
			if y < 3 or (x > 14 and y < 8): t = "snow"
			if x < 3 and y > 8: t = "water"
			if x in [8,9] and y > 2 and y < 13: t = "road"
			if (x == 6 and y in [5,6,7,8]) or (x == 13 and y in [9,10,11]): t = "blocked"
			if (x == 11 and y == 5) or (x == 16 and y == 12): t = "objective"
			row.append(t)
		terrain.append(row)

func _build_guards() -> void:
	guards = [
		{"pos":Vector2(7,5), "dir":Vector2.RIGHT, "patrol":[Vector2(7,5),Vector2(12,5)], "leg":0, "wait":0.0, "alive":true},
		{"pos":Vector2(13,9), "dir":Vector2.DOWN, "patrol":[Vector2(13,9),Vector2(13,13)], "leg":0, "wait":0.0, "alive":true},
		{"pos":Vector2(16,4), "dir":Vector2.LEFT, "patrol":[Vector2(16,4),Vector2(16,7)], "leg":0, "wait":0.0, "alive":true},
		{"pos":Vector2(10,11), "dir":Vector2.UP, "patrol":[Vector2(10,11),Vector2(6,11)], "leg":0, "wait":0.0, "alive":true}
	]

func _update_guards(delta: float) -> void:
	if alarm:
		alarm_timer += delta
	for g in guards:
		if not g.alive: continue
		if g.wait > 0.0:
			g.wait -= delta
			continue
		var dest: Vector2 = g.patrol[(g.leg + 1) % g.patrol.size()]
		var to_dest := dest - g.pos
		if to_dest.length() < 0.05:
			g.leg = (g.leg + 1) % g.patrol.size()
			g.wait = 0.8
		else:
			g.dir = to_dest.normalized()
			g.pos += g.dir * delta * (1.4 + (2.0 if alarm else 0.0))

func _update_specialists(delta: float) -> void:
	for i in specialists.size():
		var s = specialists[i]
		if not s.alive: continue
		var dest: Vector2 = targets[i]
		var to_dest := dest - Vector2(s.pos)
		if to_dest.length() > 0.03:
			var speed := CRAWL_SPEED if s.crouch else MOVE_SPEED
			var step := to_dest.normalized() * speed * delta
			if step.length() > to_dest.length(): step = to_dest
			s.pos = Vector2(s.pos) + step

func _check_detection(delta: float) -> void:
	for s in specialists:
		if not s.alive: continue
		for g in guards:
			if not g.alive: continue
			var level := _vision_level(g, Vector2(s.pos))
			if level == 2 or (level == 1 and not s.crouch and not s.get("disguised", false)):
				alarm = true
				message = "ALARM! %s bol odhalený. Presila prichádza - načítaj F9 alebo improvizuj." % s.role
				if alarm_timer > 1.5:
					s.alive = false

func _vision_level(g: Dictionary, p: Vector2) -> int:
	var v := p - g.pos
	var d := v.length()
	if d > LIGHT_RANGE or d < 0.01: return 0
	if abs(g.dir.angle_to(v.normalized())) > CONE_ANGLE * 0.5: return 0
	if _line_blocked(g.pos, p): return 0
	return 2 if d <= DARK_RANGE else 1

func _line_blocked(a: Vector2, b: Vector2) -> bool:
	for i in range(1, 12):
		var p := a.lerp(b, i / 12.0)
		var t := Vector2i(roundi(p.x), roundi(p.y))
		if _in_bounds(t) and terrain[t.y][t.x] == "blocked": return true
	return false

func _try_silent_takedown() -> void:
	var s = specialists[selected]
	for g in guards:
		if g.alive and Vector2(s.pos).distance_to(g.pos) < (4.2 if selected == 1 and s.get("ammo",0) > 0 else 1.2):
			if selected == 1: s.ammo -= 1
			g.alive = false
			bodies.append({"pos":g.pos, "hidden":false})
			message = "%s ticho neutralizuje hliadku. Telá ukry klávesom H." % s.role
			return
	message = "Žiadny cieľ v dosahu špecialistu."

func _try_hide_body() -> void:
	for b in bodies:
		if not b.hidden and Vector2(specialists[selected].pos).distance_to(b.pos) < 1.3:
			b.hidden = true
			message = "Telo je ukryté mimo patrolovacích trás."
			return
	message = "Pri špecialistovi nie je telo na ukrytie."

func _try_distract_guard() -> void:
	if not specialists[5].get("disguised", false):
		message = "Špión potrebuje uniformu (U)."
		return
	for g in guards:
		if g.alive and Vector2(specialists[5].pos).distance_to(g.pos) < 3.0:
			g.dir = (Vector2(specialists[5].pos) - g.pos).normalized()
			g.wait = 3.0
			message = "René rozpráva po nemecky a otáča zorné pole stráže."
			return

func _quick_save() -> void:
	saved_state = {"specialists": specialists.duplicate(true), "guards": guards.duplicate(true), "bodies": bodies.duplicate(true), "targets": targets.duplicate(true), "alarm": alarm, "alarm_timer": alarm_timer}
	message = "Quick Save uložený. Skúšaj odvážny postup."

func _quick_load() -> void:
	if saved_state.is_empty():
		message = "Najprv ulož pozíciu cez F5."
		return
	specialists = saved_state["specialists"].duplicate(true)
	guards = saved_state["guards"].duplicate(true)
	bodies = saved_state["bodies"].duplicate(true)
	targets = saved_state["targets"].duplicate(true)
	alarm = saved_state["alarm"]
	alarm_timer = saved_state["alarm_timer"]
	message = "Quick Load: chyba bola vymazaná, taktika pokračuje."

func _draw() -> void:
	_draw_map()
	_draw_vision()
	_draw_bodies()
	_draw_units()
	_draw_ui()

func _draw_map() -> void:
	for y in MAP_H:
		for x in MAP_W:
			var p := _tile_to_screen(Vector2(x,y))
			var poly := PackedVector2Array([p+Vector2(0,-TILE_H/2), p+Vector2(TILE_W/2,0), p+Vector2(0,TILE_H/2), p+Vector2(-TILE_W/2,0)])
			var color := {"grass":Color(0.18,0.38,0.2),"snow":Color(0.78,0.86,0.9),"water":Color(0.08,0.22,0.5),"road":Color(0.34,0.31,0.25),"blocked":Color(0.22,0.20,0.18),"objective":Color(0.55,0.42,0.12)}[terrain[y][x]]
			draw_colored_polygon(poly, color)
			var outline := PackedVector2Array(poly)
			outline.append(poly[0])
			draw_polyline(outline, Color(0,0,0,0.28), 1.0)

func _draw_vision() -> void:
	for g in guards:
		if not g.alive: continue
		var base := _tile_to_screen(g.pos)
		for r in [LIGHT_RANGE, DARK_RANGE]:
			var pts := PackedVector2Array([base])
			for i in range(-8,9):
				var ang := g.dir.angle() + i * CONE_ANGLE / 16.0
				pts.append(_tile_to_screen(g.pos + Vector2(cos(ang), sin(ang)) * r))
			draw_colored_polygon(pts, Color(0.25,1.0,0.25,0.16 if r == LIGHT_RANGE else 0.28))

func _draw_bodies() -> void:
	for b in bodies:
		if b.hidden: continue
		var p := _tile_to_screen(b.pos)
		draw_rect(Rect2(p-Vector2(12,5), Vector2(24,10)), Color(0.25,0.03,0.02))

func _draw_units() -> void:
	for g in guards:
		if g.alive:
			_draw_person(_tile_to_screen(g.pos), Color(0.18,0.18,0.16), Color(0.55,0.08,0.06), false)
	for i in specialists.size():
		var s = specialists[i]
		if not s.alive: continue
		var p := _tile_to_screen(Vector2(s.pos))
		_draw_person(p, s.color, Color.WHITE, i == selected)
		draw_string(ThemeDB.fallback_font, p + Vector2(-8, -28), str(i+1), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color.WHITE)

func _draw_person(p: Vector2, body: Color, accent: Color, active: bool) -> void:
	if active: draw_circle(p, 18, Color(1,0.85,0.2,0.28))
	draw_circle(p + Vector2(0,-18), 6, accent)
	draw_rect(Rect2(p.x-6,p.y-16,12,20), body)
	draw_line(p+Vector2(-5,2), p+Vector2(-12,12), body, 3)
	draw_line(p+Vector2(5,2), p+Vector2(12,12), body, 3)

func _draw_ui() -> void:
	var panel := Rect2(16, 16, 520, 178)
	draw_rect(panel, Color(0.025,0.03,0.035,0.82))
	draw_string(ThemeDB.fallback_font, Vector2(30,44), "Operácia Kopanice: izometrická RTT stealth stratégia", HORIZONTAL_ALIGNMENT_LEFT, -1, 20, Color(0.95,0.9,0.72))
	var s = specialists[selected]
	draw_string(ThemeDB.fallback_font, Vector2(30,74), "Aktívny: %s / %s | %s" % [s.role, s.name, s.tool], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, s.color)
	draw_string(ThemeDB.fallback_font, Vector2(30,100), "C plazenie, K tichá likvidácia, H ukryť telo, U/D špión, F5/F9 trial-and-error", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color.WHITE)
	draw_string(ThemeDB.fallback_font, Vector2(30,128), message, HORIZONTAL_ALIGNMENT_LEFT, 490, 14, Color(1,0.82,0.45) if alarm else Color(0.82,0.9,1))
	draw_string(ThemeDB.fallback_font, Vector2(30,164), "Ciele: sabotuj žlté sklady, zachráň tím, nevyvolaj alarm. Žiadne základne, žiadna výroba armád.", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.75,0.95,0.75))

func _tile_to_screen(t: Vector2) -> Vector2:
	return camera_offset + Vector2((t.x - t.y) * TILE_W * 0.5, (t.x + t.y) * TILE_H * 0.5)

func _screen_to_tile(p: Vector2) -> Vector2i:
	var q := p - camera_offset
	var x := q.x / TILE_W + q.y / TILE_H
	var y := q.y / TILE_H - q.x / TILE_W
	return Vector2i(roundi(x), roundi(y))

func _in_bounds(t: Vector2i) -> bool:
	return t.x >= 0 and t.y >= 0 and t.x < MAP_W and t.y < MAP_H
