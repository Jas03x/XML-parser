#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#define XML_EOF 0
#define XML_NODE 1
#define XML_VALUE 2

class XAttrib
{
public:
    std::string name;
    std::string value;

    XAttrib(){}

    XAttrib(const std::string& str)
    {
        size_t index = str.find('=');
        name = str.substr(0, index); // copy the name from the indices: 0 to the equal to sign's index
        value = str.substr(index + 2, str.substr(index + 2).size() - 1); // copy the value, skip the equal to sign and the 2 quotation marks
    }

    std::string toString() const {
        return (name + "=\"" + value + "\"");
    }
};

class XNode
{
private:
    const XNode* _findNode(const char* node_name) const
    {
        if(strcmp(node_name, this->name.c_str()) == 0)
            return this;

        for(unsigned int i = 0; i < this->children.size(); i++)
        {
            const XNode* res = this->children.at(i)._findNode(node_name);
            if(res != NULL) return res;
        }

        return NULL;
    }

public:
    XNode* parent;

    std::string name;
    std::string value;
    std::vector<XAttrib> attributes;
    std::vector<XNode> children;

    XNode()
    {
        this->parent = NULL;
        this->name = "";
        this->value = "";
        this->attributes = std::vector<XAttrib>();
        this->children = std::vector<XNode>();
    }

    void print() const // print the string representation in the format: name: value ...attributes...
    {
        printf("%s: [%s] ", name.c_str(), value.c_str());
        for(unsigned int i = 0; i < this->attributes.size(); i++)
            printf("%s ", this->attributes.at(i).toString().c_str());
        printf("\n");
    }

    const XNode* findNode(const char* name) const
    {
        const XNode* res = this->_findNode(name);
        if(!res) printf("Warning: Node [%s] not found.", name);
        return res;
    }

    const XAttrib* findAttribute(const char* name) const
    {
        puts("Find attibute");
        for(unsigned int i = 0; i < this->attributes.size(); i++)
        {
            puts("Attempt:");
            printf("Try %s vs %s\n", name, this->attributes.at(i).name.c_str());
            if(strcmp(this->attributes.at(i).name.c_str(), name) == 0) return &this->attributes.at(i);
        }

        printf("Warning: Attribute [%s] not found.", name);
        return NULL;
    }
};

class XFILE
{
private:
    FILE* file; // the file being read

    int read(std::string& buffer)
    {
        buffer.clear();
        int c;
        while((c = fgetc(file)) != EOF)
        {
            if(c == '>')
            {
                c = fgetc(file); // get the next character to check what kind of a line this is

                if(c != EOF && c != '\n') // move back if we have not reached the end of the file or line
                    fseek(file, -1, SEEK_CUR);

                return XML_NODE; // this must be a node since this statement was ended by '>'
            }
            else if(c == '<' && buffer.length() > 0)
            {
                return XML_VALUE; // this must be a value since this statement was ended by  '<'
            }
            else if(c != '<') // check again if this isnt '<' since the last statement checks length as well
            {
                if(c == ' ' && buffer.length() == 0)
                    continue;
                buffer += (char) c;
            }
        }

        return XML_EOF; // if we have reached here then the file is done being read
    }

public:
    XFILE(const char* str) // read the file into the node heirchy
    {
        this->file = fopen(str, "r");
        if(file == NULL) {
            printf("XML file [%s] not found!\n", str);
            throw -1;
        }
    }

    XNode* read()
    {
        XNode* root = new XNode();
        XNode* current = root;

        std::string buffer;
        int ret = 0;
        while(ret = read(buffer), ret != XML_EOF) // read the next node/value
        {
            if(buffer[0] == '?') // comment
                continue;

            else if(buffer[0] == '/') // end of node
            {
                if(buffer.substr(1) != current->name) // confirm the node names match
                    printf("[WARNING]: Name mismatch: [%s] vs [%s]\n", buffer.substr(1).c_str(), current->name.c_str());
                current = current->parent;
            }

            else if(ret == XML_VALUE) // if this is a value string, we simply set it to the current node
                current->value = buffer;

            else if(buffer.find(' ') == std::string::npos) // this node has no attributes, simply append it to the children
            {
                XNode node = XNode();
                node.name = buffer;
                node.parent = current;
                current->children.push_back(node);

                if(buffer.back() != '/') // this  node does not instant terminate, set it to the current node
                    current = &current->children.back();
                else // this node does instant terminate, make sure you remove the '/' at the end
                    current->children.back().name.pop_back();
            }

            else // this node does have attributes
            {
                char* str = (char*) malloc(buffer.length() + 1); // allocate and initalize the buffer
                if(!str) throw -1;
                strcpy(str, buffer.c_str());

                char* token = strtok(str, " "); // start tokenizing
                XNode node = XNode();
                node.name = token; // the first token is the name
                node.parent = current;

                while(true) // parse the attribute tokens
                {
                    token = strtok(NULL, " "); // get the next token
                    if(token == NULL)
                        break;

                    if(token[strlen(token) - 1] == '/') // if the current token ends with a '/', then remove it
                        token[strlen(token) - 1] = '\0';

                    XAttrib attribute = XAttrib(std::string(token));
                    node.attributes.push_back(attribute);
                }

                current->children.push_back(node);
                if(buffer.back() != '/') // if this node does not instant-terminate, set it to the current node
                    current = &current->children.back();
                free(str);
            }
        }

        return root;
    }

    ~XFILE() {
        fclose(this->file);
    }
};

void printNodeTree(const XNode* ptr, unsigned int level)
{
    if(ptr == NULL) return;
    for(unsigned int i = 0; i < ptr->children.size(); i ++)
    {
        for(unsigned int s = 0; s < level; s++) printf("_");
        ptr->children.at(i).print();
        printNodeTree(&ptr->children.at(i), level + 1);
    }
}

int main()
{
    const XNode* root = XFILE("/Users/Jas/AndroidStudioProjects/MyApplication/app/src/main/assets/models/Bear.dae").read();
    printNodeTree(root, 0);

    printf("Search for node COLLADA:\n");
    root->findNode("COLLADA")->print();

    return 0;
}
