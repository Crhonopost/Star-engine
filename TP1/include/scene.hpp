#include <glm/glm.hpp>
#include <vector>

using std::vector;

struct MeshInstance{
    vector<unsigned short> indices;
    // std::vector<std::vector<unsigned short> > triangles;
    vector<glm::vec3> indexed_vertices;
    vector<glm::vec2> tex_coords;

    bool needUpdate = false;

    
    MeshInstance(vector<unsigned short> &indices, vector<glm::vec3> &indexed_vertices, vector<glm::vec2> &tex_coords){
        this->indexed_vertices = indexed_vertices;
        this->indices = indices;
        this->tex_coords = tex_coords;
    }

    MeshInstance(){
        indices = vector<unsigned short>();
        indexed_vertices = vector<glm::vec3>();
        tex_coords = vector<glm::vec2>();
    }
};



class Mountain : public MeshInstance{
    private:

    double width, height;
    glm::vec3 position;
    size_t nbOfVertices;
    float scale;

    void generateMesh(){
        indexed_vertices.resize(0);
        indices.resize(0);
        tex_coords.resize(0);

        double edgeLengthRow = width / (double) (nbOfVertices - 1);
        double edgeLengthCol = height / (double) (nbOfVertices - 1);

        glm::vec3 offset(width / 2., 0, height / 2.);

        for(size_t i=0; i<nbOfVertices; i++){
            for(size_t j=0; j<nbOfVertices; j++){
                glm::vec3 vertexPos = glm::vec3((double)j * edgeLengthRow, 0, (double) i * edgeLengthCol) - offset + position;
                
                indexed_vertices.push_back(vertexPos);
                
                glm::vec2 v_coords(
                    (double) i / (double) (nbOfVertices - 1),
                    1. - (double) j / (double) (nbOfVertices - 1));
                tex_coords.push_back(v_coords);
            }
        }

        for(size_t i=0; i<nbOfVertices-1; i++){
            for(size_t j=1; j<nbOfVertices; j++){
                indices.push_back(i * nbOfVertices + j - 1);
                indices.push_back(i * nbOfVertices + j);
                indices.push_back((i + 1) * nbOfVertices + j - 1);


                indices.push_back(i * nbOfVertices + j);
                indices.push_back((i + 1) * nbOfVertices + j - 1);
                indices.push_back((i + 1) * nbOfVertices + j);        
            }
        }
    }


    public:

    Mountain(){
        MeshInstance();
        width = height = 1;
        position = glm::vec3(0);
        nbOfVertices = 2;
        scale = 1;

        generateMesh();
    }

    Mountain(double width, double height, glm::vec3 position, size_t nbOfVertices, float scale){
        MeshInstance();
        this->width = width;
        this->height = height;
        this->position = position;
        this->nbOfVertices = nbOfVertices;
        this->scale = scale;

        generateMesh();
    }

    void getBufferData(std::vector<float> &vertex_buffer_data){
        vertex_buffer_data.resize(0);
        for(int i=0; i<indexed_vertices.size(); i++){
            vertex_buffer_data.push_back(indexed_vertices[i].x);
            vertex_buffer_data.push_back(indexed_vertices[i].y);
            vertex_buffer_data.push_back(indexed_vertices[i].z);
        
            vertex_buffer_data.push_back(tex_coords[i].x);
            vertex_buffer_data.push_back(tex_coords[i].y);
        }
    }

    void addVerticesQuantity(int newNumber){
        nbOfVertices += newNumber;
        generateMesh();
        needUpdate = true;
    }

    int getNumberOfVertices(){
        return nbOfVertices;
    }
};


struct Scene{
    Mountain mountain;
    GLuint vertexbuffer;
    GLuint elementbuffer;
    std::vector<float> vertex_buffer_data;
    Texture::TextureManager tm;

    Scene(){};
    
    Scene(GLuint programID){
        mountain = Mountain(10, 10, glm::vec3(0, -1.5, 0), 200, 1);
        mountain.getBufferData(vertex_buffer_data);
    
        glGenBuffers(1, &vertexbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, vertex_buffer_data.size() * sizeof(float), &vertex_buffer_data[0], GL_DYNAMIC_DRAW);

        // Generate a buffer for the indices as well
        glGenBuffers(1, &elementbuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mountain.indices.size() * sizeof(unsigned short), &mountain.indices[0] , GL_DYNAMIC_DRAW);

        tm.loadTexture("../assets/images/grass.png", "texGrass", programID);
        tm.loadTexture("../assets/images/rock.png", "texRock", programID);
        tm.loadTexture("../assets/images/snowrocks.png", "texSnow", programID);

        tm.loadTexture("../assets/images/Heightmap_Rocky.png", "heightMap", programID);
    }

    void render(GLuint programID){
        if(mountain.needUpdate){
            mountain.needUpdate = false;

            mountain.getBufferData(vertex_buffer_data);

            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
            glBufferData(GL_ARRAY_BUFFER, vertex_buffer_data.size() * sizeof(float), &vertex_buffer_data[0], GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mountain.indices.size() * sizeof(unsigned short), &mountain.indices[0] , GL_DYNAMIC_DRAW);
        }




        // 1rst attribute buffer : vertices
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        // Index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

        tm.render(programID);

        // Draw the triangles !
        glDrawElements(
                    GL_TRIANGLES,      // mode
                    mountain.indices.size(),    // count
                    GL_UNSIGNED_SHORT,   // type
                    (void*)0           // element array buffer offset
                    );

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }

    void clear(){
        glDeleteBuffers(1, &vertexbuffer);
        glDeleteBuffers(1, &elementbuffer);
    }
};

