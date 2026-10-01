extends Camera2D

func _ready():
	rotation_degrees = 45  # Isometrický uhol

func _input(event):
	if event.is_action_pressed("rotate_left"):
		rotation_degrees -= 90
	elif event.is_action_pressed("rotate_right"):
		rotation_degrees += 90
