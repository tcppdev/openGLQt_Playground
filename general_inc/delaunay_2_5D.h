#include <glm/glm.hpp>
#include <Eigen/Core>

#include <algorithm>
#include <vector>
#include <stdexcept>

#include <QOpenGLContext> 
#include <QOpenGLFunctions_3_3_Core>

#include <general_inc/shader.h>
#include <general_inc/line.h>
#include <general_inc/utilities.h> // colors

#include "CDT.h"
#include "Triangulation.h"

#include "lagan/transform.h"

struct ConstrainedDelaunayContourEdges 
{
    std::vector<std::vector<std::pair<double, double>>> closed_contours; 
    std::vector<std::pair<double, double>> steiner_points = {};
    bool contains_holes = false;

    ConstrainedDelaunayContourEdges(std::vector<std::vector<std::pair<double, double>>> delaunay_edges,
                                    bool holes_present): 
                                    closed_contours(delaunay_edges), contains_holes(holes_present) {};
};

/// Class for drawing plane 2D surfaces or 2D surfaces projected on a 3D sphere using Delaunay triangulation
/// This class allows for use of the Constrained Delaunay Algorithm to draw almost arbritrary 2D shapes 
/// Assumes the polyline contour is defined along the vertex order provided
class Delaunay2_5D: protected QOpenGLFunctions_3_3_Core
{
public:
    
    Delaunay2_5D() = delete; // need to at least give some coordinates

    //  Draw a shape in 2D to include it in a 3D scene: 
    Delaunay2_5D(std::vector<ConstrainedDelaunayContourEdges> closed_contours, Color fill_color = Color::GREEN,
               bool include_wireframe = true, bool contains_holes = false)
    {
        // First and last element of contour may be 
        fill_color_ = fill_color;

        // Polygon shader
        const char* delaunay_2_5D_vertex_shader_path = "/home/t.clar/Repos/openGLQt/shaders/delaunay_2_5D.vs";
        const char* delaunay_2_5D_shader_path = "/home/t.clar/Repos/openGLQt/shaders/delaunay_2_5D.fs";
        m_delaunay_shader = new Shader(delaunay_2_5D_vertex_shader_path, delaunay_2_5D_shader_path);

        // 

        initializeOpenGLFunctions();   // Initialise current context  (required)
 
        // Setup opengl states
        setup(); 
    }

    // Draw a lat/lon contour to project it on a WGS84 sphere in a 3D scene
    Delaunay2_5D(std::vector<ConstrainedDelaunayContourEdges> contour_edges, float delta_lon, float delta_lat, double altitude,
                 Color fill_color = Color::RED, bool include_wireframe = true, bool contains_holes = false)
    {
        fill_color_ = fill_color;
        include_wireframe_ = include_wireframe;
        const char* delaunay_2_5D_vertex_shader_path = "/home/t.clar/Repos/openGLQt/shaders/delaunay_2_5D.vs";
        const char* delaunay_2_5D_shader_path = "/home/t.clar/Repos/openGLQt/shaders/delaunay_2_5D.fs";
        m_delaunay_shader = new Shader(delaunay_2_5D_vertex_shader_path, delaunay_2_5D_shader_path);

        // Draw 2D Lat/Lon surface on a 3D WGS84 Ellipsoid
        // Create steiners points
        for (ConstrainedDelaunayContourEdges& contour: contour_edges)
        {
            std::vector<double> longitude_positions;
            std::vector<double> latitude_positions;

            for (std::vector<std::pair<double, double>>& polyline_edge: contour.closed_contours)
            {
                for (std::pair<double, double>& vertex: polyline_edge)
                {
                    longitude_positions.push_back(vertex.first);
                    latitude_positions.push_back(vertex.second);
                }
            }

            // Create steiner's point
            auto min_max_longitudes = std::minmax_element(longitude_positions.begin(), longitude_positions.end());
            auto min_max_latitudes = std::minmax_element(latitude_positions.begin(), latitude_positions.end());

            std::vector<double> longitudes = make_step_vector(*min_max_longitudes.first, delta_lon, *min_max_longitudes.second);
            std::vector<double> latitudes = make_step_vector(*min_max_latitudes.first , delta_lat, *min_max_latitudes.second);
            
            for (double const& longitude: longitudes)
            {
                for (double const& latitude: latitudes)
                {
                    contour.steiner_points.push_back(std::make_pair(longitude, latitude));
                }
            }

        }

        // Do the triangulation 
        std::vector<CDT::Triangulation<double>> cdts = do_triangulation(contour_edges);

        // Setup the triangulation solution for openGL
        setup_buffer_info(cdts, true, altitude);

        // Setup buffer data
        initializeOpenGLFunctions();   // Initialise current context  (required)
        setup();
    }

    ~Delaunay2_5D() {
        // delete m_outline_lines;
        delete m_delaunay_shader;
    }

    std::vector<CDT::Triangulation<double>> do_triangulation(std::vector<ConstrainedDelaunayContourEdges> contours)
    {
        std::vector<CDT::Triangulation<double>> cdts;

        for (ConstrainedDelaunayContourEdges const contour: contours)
        {
            CDT::Triangulation<double> cdt;
            std::vector<CDT::V2d<double>> vertices;
            std::vector<CDT::Edge> edges;

            std::vector<std::vector<std::pair<double, double>>> contour_edges = contour.closed_contours;
            int index_number = 0;

            for (std::vector<std::pair<double, double>> const& polyline_edge: contour_edges)
            {
                int polyline_start_index = index_number;
                for (std::size_t i = 0; i < polyline_edge.size(); i++)
                {
                    std::pair<double, double> vertex  = polyline_edge[i];
                    vertices.push_back(CDT::V2d<double>::make(vertex.first, vertex.second));

                    if (i < polyline_edge.size() - 1)
                    {
                        edges.push_back(CDT::Edge(index_number, index_number + 1));
                    }
                    else {  // Close the polyline
                        edges.push_back(CDT::Edge(index_number, polyline_start_index));
                    }
                    
                    index_number++;
                }
            }

            // Insert steiner's points (if exist)
            if (contour.steiner_points.size() > 0){
                for (std::pair<double, double> const& steiner_point: contour.steiner_points)
                {
                    vertices.push_back(CDT::V2d<double>::make(steiner_point.first, steiner_point.second));
                }

            }

            CDT::RemoveDuplicatesAndRemapEdges(vertices, edges);  // Ensure no duplicates points/edges exists
            cdt.insertVertices(vertices);
            cdt.insertEdges(edges);
            

            // Process cdt
            if (contour.contains_holes)
            {
                cdt.eraseOuterTrianglesAndHoles();
            }
            else 
            {
                cdt.eraseOuterTriangles();
            }

            cdts.push_back(cdt);
            
        }


        return cdts;
    };

    void setup_buffer_info(std::vector<CDT::Triangulation<double>> cdts, bool project_on_sphere, double altitude)
    {

        int starting_index_number = 0;
        std::size_t index_offset = 0;

        for (CDT::Triangulation<double> const& cdt: cdts)
        {
            std::vector<Eigen::Vector3d> transformed_vertices; 

            auto triags = cdt.triangles;
            auto vertx = cdt.vertices;

            // Project vertex on 3D WGS84 sphere?
            for (auto vertex: vertx){
                Eigen::Vector3d point_lla = {static_cast<double>(vertex.x), static_cast<double>(vertex.y), altitude};
                Eigen::Vector3d ecef_point = lagan::lla2ecef(point_lla);
                transformed_vertices.push_back(ecef_point);
            }

            std::size_t i = 0;
            
            // std::cout << starting_index_number << std::endl;
            for (auto triangle: triags)
            {
                element_vertex_count_[index_offset + i] = (GLsizei)triangle.vertices.size();
                elements_start_indexes_[index_offset + i] += static_cast<GLuint>(starting_index_number + i*triangle.vertices.size());
                
                for (auto vertex_index: triangle.vertices)
                {
                    Eigen::Vector3d coordinate = transformed_vertices[vertex_index];
                    SimpleVertex vertex;
                    glm::vec3 vector; 
                    // positions
                    vector.x = coordinate[0];
                    vector.y = coordinate[1];
                    vector.z = coordinate[2];
                    vertex.Position = vector;

                    vertices_.push_back(vertex);
                }

                i++;
                triangles_count_ += 1;
                
            }

            index_offset += i;
            starting_index_number += i*3;
            
        }

    }
    
    void setup()
    {
        // Create the buffers and array:
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);  

        // load data into buffers
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(SimpleVertex), &vertices_[0], GL_STATIC_DRAW);  

        // set the vertex attribute pointers:
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)0);
    }

    void draw(glm::mat4 view_matrix = glm::mat4(1.0f), glm::mat4 projection_matrix = glm::mat4(1.0f))
    {
        m_delaunay_shader->use();  // Bind shader

        // Set the uniforms:
        glm::vec4 ourcolor = get_color(fill_color_);  // get the color
        m_delaunay_shader->setVec4("ourColor", ourcolor); // Set uniform
        m_delaunay_shader->setMat4("view", view_matrix);
        m_delaunay_shader->setMat4("projection", projection_matrix);
    
        // Draw polygons
        glEnable(GL_MULTISAMPLE);  // Antialiasing
        glBindVertexArray(vao_);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0, 1.0f); // move polygon backward
        glMultiDrawArrays(GL_TRIANGLES, elements_start_indexes_, element_vertex_count_, triangles_count_); //
        glDisable(GL_POLYGON_OFFSET_FILL);
        
        if (include_wireframe_) {
            // Turn on wireframe mode
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            m_delaunay_shader->setVec4("ourColor", get_color(Color::BLACK));
            glMultiDrawArrays(GL_TRIANGLES, elements_start_indexes_, element_vertex_count_, triangles_count_); //
            // Turn off wireframe mode
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glBindVertexArray(0);  // Unbind vao
    }

private:
    Color fill_color_ = Color::GREEN;
    Color linecolor_ = Color::BLUE;
    float linewidth_ = DEFAULT_LINE_WIDTH;
    std::vector<std::vector<Eigen::Vector3f>> polygons_;
    GLuint triangles_count_ = 0;
    GLsizei elements_start_indexes_[MAX_FEATURES];
    GLsizei element_vertex_count_[MAX_FEATURES];
    std::vector<SimpleVertex> vertices_;
    unsigned int vao_, vbo_;
    
    Shader* m_delaunay_shader;
    Line* outline_lines_ptr;

    bool include_wireframe_ = false;
};