select v.mesh, part, vertex_id, 
coalesce(bone_1_weight, 0) + coalesce(bone_2_weight,0) + coalesce(bone_3_weight,0) + coalesce(bone_4_weight, 0) as weight_sum,
b1.name as bone_1, bone_1_weight, b2.name as bone_2, bone_2_weight, b3.name as bone_3, bone_3_weight, b4.name as bone_4, bone_4_weight 
from vertices v
left join bones b1 on bone_1_id = b1.bone_id and b1.mesh = v.mesh
left join bones b2 on bone_2_id = b2.bone_id and b2.mesh = v.mesh
left join bones b3 on bone_3_id = b3.bone_id and b3.mesh = v.mesh
left join bones b4 on bone_4_id = b4.bone_id and b4.mesh = v.mesh
--where weight_sum > 1.0000001
--or weight_sum < 0.999999
