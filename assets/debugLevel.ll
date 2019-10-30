map {
  _filename = "debugLevel.png"
}

texture {
  _name = "walls_tex"
  _filename = "tileset/Sliced/world_24x24/oryx_16bit_fantasy_world_59.png"
}

texture {
  _name = "player_tex"
  _filename = "tileset/Sliced/creatures_24x24/oryx_16bit_fantasy_creatures_01.png"
}

texture {
  _name = "enemy_tex"
  _filename = "tileset/Sliced/creatures_24x24/oryx_16bit_fantasy_creatures_04.png"
}

tile {
  _name = "walls_tile"
  _texture = "walls_tex"
}

tile {
  _name = "player_tile"
  _texture = "player_tex"
}

tile {
  _name = "enemy_tile"
  _texture = "enemy_tex"
}

entity {
  _type = "Walls"
  _tile = "walls_tile"
  _patterned = true
  _colorBinding = #FFFFFF
}

entity {
  _type = "Player"
  _tile = "player_tile"
  CompPos = 20/20
  CompDir = 0/1
  CompMana = 100
  CompStamina = 100
}

entity {
  _type = "Enemy"
  _tile = "enemy_tile"
  _colorBinding = #FF0000
  CompHealth = 30
  CompArmor = 0/0
  CompDamage = 10/10
  CompVisionRadius = 8
}
