extends CharacterBody2D

var target_position = null
var speed = 200

func _input(event):
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		target_position = get_global_mouse_position()

func _physics_process(delta):
	if target_position:
		var direction = (target_position - global_position).normalized()
		velocity = direction * speed
		move_and_slide()
		if global_position.distance_to(target_position) < 10:
			target_position = null
			velocity = Vector2.ZERO
