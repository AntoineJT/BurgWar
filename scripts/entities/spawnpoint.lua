ENTITY.IsNetworked = false

function ENTITY:Initialize()
	if (EDITOR) then
		self:AddSprite({
			Scale = Vec2(0.33, 0.33),
			TexturePath = "spawnpoint.png"
		})
	end
end
