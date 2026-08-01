func _input(event):
	if event.is_action_pressed("rotate_left"):
		get_node("/root/Main/Camera2D").rotation_degrees -= 90
	elif event.is_action_pressed("rotate_right"):
		get_node("/root/Main/Camera2D").rotation_degrees += 90
