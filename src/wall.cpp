#include "wall.h"

Wall::Wall(glm::vec2 normal,
           glm::vec2 startPoint,
           glm::vec2 endPoint)
   : mNormal(glm::normalize(normal))
   , mC(-glm::dot(mNormal, startPoint))
   , mStartPoint(startPoint)
   , mEndPoint(endPoint)
{

}
