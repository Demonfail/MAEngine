///MAE_PhysBodySetLinearFactorVector(body,factor);

/**
 * Description:
 *   Sets the linear factor of a physics body. This can be used to restrict movement per axis. You could limit the physics engine to a 2D plane, for example.
 *
 * Arguments:
 *   [1] - Body
 *
 * Returns:
 *   Success
 */

var factor = argument1;

return external_call(global._MAB_BodySetLinearFactor, argument0, factor[0], factor[1], factor[2]);
