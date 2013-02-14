---------------------------------------------------
-- Refueling
-- Steals an enemy's HP. Ineffective against undead.
---------------------------------------------------

require("/scripts/globals/settings");
require("/scripts/globals/status");
require("/scripts/globals/monstertpmoves");

---------------------------------------------------

function OnMobSkillCheck(target,mob,skill)
	return 0;
end;

function OnMobWeaponSkill(target, mob, skill)
	skill:setMsg(MSG_BUFF);
	local typeEffect = EFFECT_HASTE;
	mob:delStatusEffect(typeEffect);
	mob:addStatusEffect(typeEffect,102,0,300);
	return typeEffect;
end
