#include "Entity.h"

Entity::Entity()
{
    entityType = PLATFORM;
    isStatic = true;
    isActive = true;
    position = glm::vec3(0);
    speed = 0;
    width = 1;
    height = 1;
}
void Entity::UpdatePos(const float incrementX, const float incrementY)
{
    position.x += incrementX;
    position.y += incrementY;
}

//Very useful method but had to modify the implementation 
//a bit that way its not just drawing things of a standard size at the center of the screen
void Entity::Render(ShaderProgram *program, glm::vec3 sizing) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::scale(modelMatrix, sizing);
    program->SetModelMatrix(modelMatrix);
    
    float vertices[]  = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

bool Entity::CheckCollision(Entity other)
{
    float xdist = fabs(position.x - other.position.x) - ((width + other.width) / 2.0f);
    float ydist = fabs(position.y - other.position.y) - ((height + other.height) / 2.0f);
    if (xdist < 0 && ydist < 0)
    {
        return true;
    }
    return false;
}

void Entity::CheckCollisionsY(Entity* objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++)
    {
        Entity object = objects[i];
        if (CheckCollision(object))
        {
            float ydist = fabs(position.y - object.position.y);
            float penetrationY = fabs(ydist - (height / 2) - (object.height / 2));
            if (velocity.y > 0) {
                collidedTop = true; //need to actually record the collision side
                position.y -= penetrationY;
                velocity.y = 0;
            }
            else if (velocity.y < 0) {
                collidedBottom = true;
                position.y += penetrationY;
                velocity.y = 0;
            }
        }
    }
}

void Entity::CheckCollisionsX(Entity* objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++)
    {
        Entity object = objects[i];
        if (CheckCollision(object))
        {
            float xdist = fabs(position.x - object.position.x);
            float penetrationX = fabs(xdist - (width / 2) - (object.width / 2));
            if (velocity.x > 0) {
                collidedRight = true; //need to actually record the collision side
                position.x -= penetrationX;
                velocity.x = 0;
            }
            else if (velocity.x < 0) {
                collidedLeft = true; //need to actually record the collision side
                position.x += penetrationX;
                velocity.x = 0;
            }
        }
    }
}

void Entity::Update(float deltaTime, Entity* objects, int objectCount)
{
    velocity += acceleration * deltaTime;
    position.y += velocity.y * deltaTime; // Move on Y
    CheckCollisionsY(objects, objectCount); // Fix if needed
    position.x += velocity.x * deltaTime; // Move on X
    CheckCollisionsX(objects, objectCount); // Fix if needed
}