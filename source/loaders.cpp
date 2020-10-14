#include "loaders.h"

TiledManager::TiledManager()
{
	this->name = "Tiled level loader [CSV]";
	this->autor = "Kolxoz";
	this->version = "1.0";
	this->description = "This parser is designed to load maps created in Tiled 1.3.2 in CSV mode";
}

TileLayer* TiledManager::loadTileLayer(XMLElement* layer)
{
	// ��� ���� 
	string layerName = layer->Attribute("name");
	
	// ������ ����
	TileLayer* currentLayer = new TileLayer(layerName);

	// �������� �����
	int offset_x = 0, offset_y = 0;
	if (layer->Attribute("offsetx") && layer->Attribute("offsety"))
	{
		offset_x = atoi(layer->Attribute("offsetx"));
		offset_y = atoi(layer->Attribute("offsety"));
	}
	currentLayer->setOffset(offset_x, offset_y);

	// ������ � �������
	XMLElement *data = layer->FirstChildElement("data");
	XMLElement *chunk = data->FirstChildElement("chunk");

	while (chunk)
	{
		int chunk_w = atoi(chunk->Attribute("width"));		// ������ �����
		int chunk_h = atoi(chunk->Attribute("height"));		// ������ �����
		int chunk_x = atoi(chunk->Attribute("x"));			// ������� ����� �� X
		int chunk_y = atoi(chunk->Attribute("y"));			// ������� ����� �� Y

		char* ch = (char*)chunk->GetText();

		/*for (int y = 0; y < chunk_h; y++)
		{
			for (int x = 0; x < chunk_w; x++)
			{
				int x_pos = (chunk_x + x) * tilewidth;
				int y_pos = (chunk_y + y) * tileheight;

				char* num = strtok(ch, " ,\n");
				int index = atoi(num) - 1;

				if (index >= 0)
				{
					cap::Texture* texture = getTexture(index);
					if (texture) currentLayer->addTile(textures[index]->toSprite(), x_pos, y_pos);
					else Script::print_log("Error! Tile with index " + to_string(index) + " is not loaded!");
				}
			}
		}*/
		chunk = chunk->NextSiblingElement("chunk"); // ��������� ����
	}
	return currentLayer;
}

ObjectLayer* TiledManager::loadObjectLayer(XMLElement* layer)
{
	// ��� ���� � ������-��������
	string layerName = layer->Attribute("name");
	ObjectLayer* currentLayer = new ObjectLayer(layerName);

	XMLElement *object = layer->FirstChildElement("object");
	while (object)
	{
		Entity* obj = nullptr;

		string name = object->Attribute("name");
		string type = object->Attribute("type");

		// ���� � ������� ���� ���
		if (!type.empty())
		{
            LuaRef constr = Script::getVar(type);
			if (constr.isTable())
			{
				LuaRef lua_obj = constr(name);
				if (lua_obj.isUserdata())
				{
                    // NOT FINISHED
				}
				obj = lua_obj["__parent"].cast<Entity*>();
			}
			else
			{
				// ���� ��� �� �������
				Script::print_log("Error! Don't such class '" + type + "', object is not loaded");
				continue;
			}
		}

		// ���� � ������� ���� ��������
		const char* gid = object->Attribute("gid");
		if (gid)
		{
			// ���� ������ �� ������ - ������
			if (!obj) obj = new DrawableEntity(name);

			// ���� ������ ������������ - ����������� ��� ��������
			if (obj->getType() == CAP_DRAWABLE_ENTTY)
			{/*
				int index = atoi(gid) - 1;
				cap::Texture* texture = getTexture(index);
				if(texture)((DrawableEntity*)obj)->setTexture(texture->toSprite());
				else Script::print_log("Warning! Object type " + type + " is not drawable, texture is not seted!");*/
			}
		}

		// ������� �������
		int x = atoi(object->Attribute("x"));
		int y = atoi(object->Attribute("y"));

		// ������� ������� (����� �������������)
		int w = 0, h = 0;
		if (object->Attribute("width") && object->Attribute("height"))
		{
			w = atoi(object->Attribute("width"));
			h = atoi(object->Attribute("height"));
			
			// ������� ������� ���� ���� ��������
			if (gid) y -= h;

			// ���� ������ �� ������ - ������
			if (!obj) obj = new RectEntity(name);

			// ������� � ������
			((RectEntity*)obj)->setSize(Point(w, h));
		}

		// ���� ������ �� ������ - ������
		if (!obj) obj = new PointEntity(name);

		// ������������� �������
		((PointEntity*)obj)->setPosition(Point(x, y));

		// �������� �������
		if (object->FirstChildElement("properties"))
		{
			XMLElement *prop = object->FirstChildElement("properties")->FirstChildElement("property");
			while (prop)
			{
				string name = prop->Attribute("name");
				string value = prop->Attribute("value");

                obj->self[name] = Script::eval(value);
				prop = prop->NextSiblingElement("property");
			}
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Put object on layer
		currentLayer->addObject(obj);
		object = object->NextSiblingElement("object");
	}
	return currentLayer;
}

GroupLayer* TiledManager::loadGroupLayer(XMLElement* group)
{
	// ������ ������
    GroupLayer* currentGroup = new GroupLayer(group->Attribute("name"));

    // Group offset
    int offset_x = group->Attribute("offsetx") ? atoi(group->Attribute("offsetx")) : 0;
    int offset_y = group->Attribute("offsety") ? atoi(group->Attribute("offsety")) : 0;
    currentGroup->setOffset(offset_x, offset_y);

	// ������ �� ������
	auto layer = group->FirstChildElement();
	while (layer)
	{
		string layerType = layer->Value();

		// �������� ����
		if (layerType == "group") currentGroup->addContainer(loadGroupLayer(layer));
		else if (layerType == "layer") currentGroup->addContainer(loadTileLayer(layer));
		else if (layerType == "objectgroup") currentGroup->addContainer(loadGroupLayer(layer));

		// ��������� ����
		layer = layer->NextSiblingElement(); 
	}

	return currentGroup;
}

Level* TiledManager::loadLevel(const string& name)
{
	// Loading level file
	string path = name + ".tmx";

	XMLDocument doc;
	if (doc.LoadFile(path.c_str()) == XML_SUCCESS)
	{
		XMLElement* map = doc.RootElement();
		if (map)
		{
			Level* lvl = new Level(name);

			tilewidth = atoi(map->Attribute("tilewidth"));
			tileheight = atoi(map->Attribute("tileheight"));

			// ��������� �����
			XMLElement* elem = map->FirstChildElement();
			while (elem)
			{
				string name = elem->Value();
				if (name == "group") lvl->addContainer(loadGroupLayer(elem));
				else if (name == "layer") lvl->addContainer(loadTileLayer(elem));
				else if (name == "objectgroup") lvl->addContainer(loadObjectLayer(elem));

				elem = elem->NextSiblingElement();
			}
			return lvl;
		}
	}
	else print_error(doc, path);

	return nullptr;
}

vector<Tileset*> TiledManager::loadTilesetsForLevel(const string& name)
{
	vector<Tileset*> list;

	// Loading level file
	string path = name + ".tmx";

	// Tileset paths loading
	XMLDocument doc, tsx;
	if (doc.LoadFile(path.c_str()) == XML_SUCCESS)
	{
		XMLElement* map = doc.RootElement();
		if (map)
		{
			XMLElement* tileset = map->FirstChildElement("tileset");
			while (tileset)
			{
				string source = tileset->Attribute("source");
				source = CAP_GAMEDATA_DIR + source.substr(3);

				Tileset* tiles = loadTileset(source);
				list[tiles->getName()] = tiles;

				tileset = tileset->NextSiblingElement("tileset");
			}
		}
	}
	else print_error(doc, path);

	return list;
}

void TiledManager::print_error(XMLDocument& doc, string path)
{
	string error_name = doc.ErrorName();
	string error_line = to_string(doc.ErrorLineNum());

	Script::print_log("Error! File '" + path + "' is not loaded! ");
	Script::print_log("Reason: " + error_name);
	Script::print_log("Line: " + error_line);
}

Tileset* TiledManager::loadTileset(const string& path)
{
	Tileset* tileset = nullptr;
	XMLDocument doc;
	if (doc.LoadFile(path.c_str()) == XML_SUCCESS)
	{
		XMLElement* root = doc.RootElement();
		XMLElement* image = root->FirstChildElement("image");
		sf::Texture* texture = new sf::Texture();
		string source = image->Attribute("source");
		source = CAP_GAMEDATA_DIR + source.substr(3);
		
		// Image loading
		if (!texture->loadFromFile(source))
		{
			delete texture;
			Script::print_log("Error. File '" + source + "' not found! Tileset not loaded!");
			return nullptr;
		}
		Point tile_size = { root->FloatAttribute("tilewidth"), root->FloatAttribute("tilewidth")};
		int tilecount = root->IntAttribute("tilecount");
		int columns = root->IntAttribute("columns");
		int rows = tilecount / columns;

		// Create tileset
		tileset = new Tileset(root->Attribute("name"));
		tileset->subdivide(texture, Point(columns, rows), tile_size, root->IntAttribute("spacing", 0), root->IntAttribute("margin", 0));
	}
	else
	{
		string error_name = doc.ErrorName();
		string error_line = to_string(doc.ErrorLineNum());

		Script::print_log("Error! File '" + path + "' is not loaded! ");
		Script::print_log("Reason: " + error_name);
		Script::print_log("Line: " + error_line);
	}

	return tileset;
}