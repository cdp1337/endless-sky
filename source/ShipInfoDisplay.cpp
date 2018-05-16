/* ShipInfoDisplay.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipInfoDisplay.h"

#include "Color.h"
#include "Depreciation.h"
#include "FillShader.h"
#include "Format.h"
#include "GameData.h"
#include "Outfit.h"
#include "Ship.h"
#include "Table.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <iostream>

using namespace std;



ShipInfoDisplay::ShipInfoDisplay(const Ship &ship, const Depreciation &depreciation, int day)
{
	Update(ship, depreciation, day);
}



// Call this every time the ship changes.
void ShipInfoDisplay::Update(const Ship &ship, const Depreciation &depreciation, int day)
{
	UpdateDescription(ship.Description(), ship.Attributes().Licenses(), true);
	UpdateAttributes(ship, depreciation, day);
	UpdateOutfits(ship, depreciation, day);
	
	maximumHeight = max(descriptionHeight, max(attributesHeight, outfitsHeight));
}



int ShipInfoDisplay::OutfitsHeight() const
{
	return outfitsHeight;
}



int ShipInfoDisplay::SaleHeight() const
{
	return saleHeight;
}



// Draw each of the panels.
void ShipInfoDisplay::DrawAttributes(const Point &topLeft) const
{
	Point point = Draw(topLeft, attributeLabels, attributeValues);
	
	// Get standard colors to draw with.
	const Color &labelColor = *GameData::Colors().Get("medium");
	const Color &valueColor = *GameData::Colors().Get("bright");
	
	Table table;
	table.AddColumn(10, Table::LEFT);
	table.AddColumn(WIDTH - 90, Table::RIGHT);
	table.AddColumn(WIDTH - 10, Table::RIGHT);
	table.SetHighlight(0, WIDTH);
	table.DrawAt(point);
	table.DrawGap(10.);
	
	table.Advance();
	table.Draw("energy", labelColor);
	table.Draw("heat", labelColor);
	
	for(unsigned i = 0; i < tableLabels.size(); ++i)
	{
		CheckHover(table, tableLabels[i]);
		table.Draw(tableLabels[i], labelColor);
		table.Draw(energyTable[i], valueColor);
		table.Draw(heatTable[i], valueColor);
	}
}



void ShipInfoDisplay::DrawOutfits(const Point &topLeft) const
{
	Draw(topLeft, outfitLabels, outfitValues);
}



void ShipInfoDisplay::DrawSale(const Point &topLeft) const
{
	Draw(topLeft, saleLabels, saleValues);
	
	const Color &color = *GameData::Colors().Get("medium");
	FillShader::Fill(topLeft + Point(.5 * WIDTH, saleHeight + 8.), Point(WIDTH - 20., 1.), color);
}



void ShipInfoDisplay::UpdateAttributes(const Ship &ship, const Depreciation &depreciation, int day)
{
	bool isGeneric = ship.Name().empty() || ship.GetPlanet();
	
	attributeLabels.clear();
	attributeValues.clear();
	attributesHeight = 20;
	
	const Outfit &attributes = ship.Attributes();
	const map<const Outfit *, int> &outfits = ship.Outfits();
	
	// Load all the attributes and their values along with the base values of this ship.
	// This won't be correct some such as hyperdrive or unplunderable, but those special cases will be ignored.
	map<string, double> attributeMapBase;
	map<string, double> attributeMapUsed;
	
	for(auto const &a : ship.BaseAttributes().Attributes()) {
		attributeMapBase[a.first] = a.second;
	}
	
	for(auto const &a : attributes.Attributes()) {
		attributeMapUsed[a.first] = a.second;
	}
	
	// Use the outfit to adjust the base correctly; it's needed for outfits that increase max capacity.
	for(const auto &o : outfits) {
		for(auto const &a : o.first->Attributes()) {
			double attval = o.first->Get(a.first);
			
			if(attval > 0) {
				attributeMapBase[a.first] += (o.second * attval);
			}
		}
	}
	
	
	int64_t fullCost = ship.Cost();
	int64_t depreciated = depreciation.Value(ship, day);
	if(depreciated == fullCost) {
		attributeLabels.push_back("cost:");
	}
	else
	{
		ostringstream out;
		out << "cost (" << (100 * depreciated) / fullCost << "%):";
		attributeLabels.push_back(out.str());
	}
	attributeValues.push_back(Format::Number(depreciated));
	attributesHeight += 20;
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	
	attributeLabels.push_back("Shields:");
	if(attributeMapBase["shield generation"]) {
		attributeValues.push_back(
			Format::Number(attributeMapBase["shields"])
			+ " ( "
			+ Format::Number(60. * attributeMapBase["shield generation"])
			+ " )"
		);
	}
	else
	{
		attributeValues.push_back(Format::Number(attributeMapBase["shields"]));
	}
	attributesHeight += 20;
	
	
	attributeLabels.push_back("Hull:");
	if(attributeMapBase["hull repair rate"]) {
		attributeValues.push_back(
			Format::Number(attributeMapBase["hull"])
			+ " ( "
			+ Format::Number(60. * attributeMapBase["hull repair rate"])
			+ " )"
		);
	}
	else
	{
		attributeValues.push_back(Format::Number(attributeMapBase["hull"]));
	}
	attributesHeight += 20;
	
	
	double emptyMass = ship.Mass();
	attributeLabels.push_back(isGeneric ? "mass with no cargo:" : "mass:");
	attributeValues.push_back(Format::Number(emptyMass));
	attributesHeight += 20;
	attributeLabels.push_back(isGeneric ? "cargo space:" : "cargo:");
	if(isGeneric)
		attributeValues.push_back(Format::Number(attributes.Get("cargo space")));
	else
		attributeValues.push_back(Format::Number(ship.Cargo().Used())
			+ " / " + Format::Number(attributes.Get("cargo space")));
	attributesHeight += 20;
	attributeLabels.push_back("required crew / bunks:");
	attributeValues.push_back(Format::Number(ship.RequiredCrew())
		+ " / " + Format::Number(attributes.Get("bunks")));
	attributesHeight += 20;
	attributeLabels.push_back(isGeneric ? "fuel capacity:" : "fuel:");
	double fuelCapacity = attributes.Get("fuel capacity");
	if(isGeneric)
		attributeValues.push_back(Format::Number(fuelCapacity));
	else
		attributeValues.push_back(Format::Number(ship.Fuel() * fuelCapacity)
			+ " / " + Format::Number(fuelCapacity));
	attributesHeight += 20;
	
	double fullMass = emptyMass + (isGeneric ? attributes.Get("cargo space") : ship.Cargo().Used());
	isGeneric &= (fullMass != emptyMass);
	double forwardThrust = attributes.Get("thrust") ? attributes.Get("thrust") : attributes.Get("afterburner thrust");
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	attributeLabels.push_back(isGeneric ? "movement, full / no cargo:" : "movement:");
	attributeValues.push_back(string());
	attributesHeight += 20;
	attributeLabels.push_back("max speed:");
	attributeValues.push_back(Format::Number(60. * forwardThrust / attributes.Get("drag")));
	attributesHeight += 20;
	
	attributeLabels.push_back("acceleration:");
	if(!isGeneric)
		attributeValues.push_back(Format::Number(3600. * forwardThrust / fullMass));
	else
		attributeValues.push_back(Format::Number(3600. * forwardThrust / fullMass)
			+ " / " + Format::Number(3600. *forwardThrust / emptyMass));
	attributesHeight += 20;
	
	attributeLabels.push_back("turning:");
	if(!isGeneric)
		attributeValues.push_back(Format::Number(60. * attributes.Get("turn") / fullMass));
	else
		attributeValues.push_back(Format::Number(60. * attributes.Get("turn") / fullMass)
			+ " / " + Format::Number(60. * attributes.Get("turn") / emptyMass));
	attributesHeight += 20;
	
	
	// Print a spacer between the previous section and this one.
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	
	
	// Listing of labels to key names to calculate from.
	const vector<string> atts = {
		"minibays",
		"bays",
		"outfit space",
		"armory space",
		"weapon capacity",
		"engine capacity",
		"gun ports",
		"turret mounts"
	};
	
	for(unsigned i = 0; i < atts.size(); i ++)
	{
		if(attributeMapBase[ atts[i] ] > 0) {
			// Only draw this attribute if the total available is > 0.  I don't care about turret slots on a shuttle!
			attributeLabels.push_back( Outfit::ATTRIBUTES.at(atts[i]) + ":" );
			attributeValues.push_back(
				Format::Number(attributeMapUsed[ atts[i] ])
				+ " / " 
				+ Format::Number(attributeMapBase[ atts[i] ])
			);
			attributesHeight += 20;
		}
	}
	
	if(ship.BaysFree(false))
	{
		attributeLabels.push_back("drone bays:");
		attributeValues.push_back(to_string(ship.BaysFree(false)));
		attributesHeight += 20;
	}
	if(ship.BaysFree(true))
	{
		attributeLabels.push_back("fighter bays:");
		attributeValues.push_back(to_string(ship.BaysFree(true)));
		attributesHeight += 20;
	}
	
	tableLabels.clear();
	energyTable.clear();
	heatTable.clear();
	// Skip a spacer and the table header.
	attributesHeight += 30;
	
	tableLabels.push_back("idle:");
	energyTable.push_back(Format::Number(
		60. * (attributes.Get("energy generation")
			+ attributes.Get("solar collection")
			- attributes.Get("energy consumption")
			- attributes.Get("cooling energy"))));
	double efficiency = ship.CoolingEfficiency();
	heatTable.push_back(Format::Number(
		60. * (attributes.Get("heat generation")
			- efficiency * (attributes.Get("cooling") + attributes.Get("active cooling")))));
	attributesHeight += 20;
	tableLabels.push_back("moving:");
	energyTable.push_back(Format::Number(
		-60. * (max(attributes.Get("thrusting energy"), attributes.Get("reverse thrusting energy"))
			+ attributes.Get("turning energy")
			+ attributes.Get("afterburner energy"))));
	heatTable.push_back(Format::Number(
		60. * (max(attributes.Get("thrusting heat"), attributes.Get("reverse thrusting heat"))
			+ attributes.Get("turning heat")
			+ attributes.Get("afterburner heat"))));
	attributesHeight += 20;
	double firingEnergy = 0.;
	double firingHeat = 0.;
	for(const auto &it : ship.Outfits())
		if(it.first->IsWeapon() && it.first->Reload())
		{
			firingEnergy += it.second * it.first->FiringEnergy() / it.first->Reload();
			firingHeat += it.second * it.first->FiringHeat() / it.first->Reload();
		}
	tableLabels.push_back("firing:");
	energyTable.push_back(Format::Number(-60. * firingEnergy));
	heatTable.push_back(Format::Number(60. * firingHeat));
	attributesHeight += 20;
	double shieldEnergy = attributes.Get("shield energy");
	double hullEnergy = attributes.Get("hull energy");
	tableLabels.push_back((shieldEnergy && hullEnergy) ? "shields / hull:" :
		hullEnergy ? "repairing hull:" : "charging shields:");
	energyTable.push_back(Format::Number(-60. * (shieldEnergy + hullEnergy)));
	double shieldHeat = attributes.Get("shield heat");
	double hullHeat = attributes.Get("hull heat");
	heatTable.push_back(Format::Number(60. * (shieldHeat + hullHeat)));
	attributesHeight += 20;
	tableLabels.push_back("max:");
	energyTable.push_back(Format::Number(attributes.Get("energy capacity")));
	heatTable.push_back(Format::Number(60. * ship.HeatDissipation() * ship.MaximumHeat()));
	// Pad by 10 pixels on the top and bottom.
	attributesHeight += 30;
}



void ShipInfoDisplay::UpdateOutfits(const Ship &ship, const Depreciation &depreciation, int day)
{
	outfitLabels.clear();
	outfitValues.clear();
	outfitsHeight = 20;
	
	map<string, map<string, int>> listing;
	for(const auto &it : ship.Outfits())
		listing[it.first->Category()][it.first->Name()] += it.second;
	
	for(const auto &cit : listing)
	{
		// Pad by 10 pixels before each category.
		if(&cit != &*listing.begin())
		{
			outfitLabels.push_back(string());
			outfitValues.push_back(string());
			outfitsHeight += 10;
		}
				
		outfitLabels.push_back(cit.first + ':');
		outfitValues.push_back(string());
		outfitsHeight += 20;
		
		for(const auto &it : cit.second)
		{
			outfitLabels.push_back(it.first);
			outfitValues.push_back(to_string(it.second));
			outfitsHeight += 20;
		}
	}
	
	
	int64_t totalCost = depreciation.Value(ship, day);
	int64_t chassisCost = depreciation.Value(GameData::Ships().Get(ship.ModelName()), day);
	saleLabels.clear();
	saleValues.clear();
	saleHeight = 20;
	
	saleLabels.push_back("This ship will sell for:");
	saleValues.push_back(string());
	saleHeight += 20;
	saleLabels.push_back("empty hull:");
	saleValues.push_back(Format::Number(chassisCost));
	saleHeight += 20;
	saleLabels.push_back("  + outfits:");
	saleValues.push_back(Format::Number(totalCost - chassisCost));
	saleHeight += 5;
}
