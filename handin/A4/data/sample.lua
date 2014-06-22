
mat1 = gr.material({0.7, 1.0, 0.7}, {0.5, 0.7, 0.5}, 25)
whitemat = gr.material({1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, 10)

scene_root = gr.node('root')

s1 = gr.nh_sphere('s1', {0, 0, -100}, 6)
scene_root:add_child(s1)
s1:set_material(whitemat)

white_light = gr.light({-100.0, 150.0, 400.0}, {0.9, 0.9, 0.9}, {1, 0, 0})

gr.render(scene_root, 'sample.png', 256, 256,
  {0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 50,
  {0.3, 0.3, 0.3}, {white_light})
